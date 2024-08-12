//=============================================================================
// Author: Jared Simons
// Date Created: 7/2/2024
// 
// Purpose: Methods and utilities for handling Vulkan shader data.
//=============================================================================

#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Systems/IMemory.h"
#include "Systems/IFileSystem.h"

#include <Math/VertexTypes.h> // TEMP


bool VulkanShader::CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader)
{
	if (!pOutShader)
	{
		LOG_ERROR("CreateVulkanShader() -> Requies a pointer to a valid VulkanShader object.");
		return false;
	}

	// Create shader modules

	// Vertex shader module
	FileHandle_t vertShaderFile;
	if (!gpFileSystem->OpenFile("Assets/Shaders/vert.spv", FILEMODE_READ, true, &vertShaderFile))
	{
		LOG_ERROR("Failed to read vertex shader file");
		return false;
	}

	size_t vertShaderFileSize = gpFileSystem->FileSize(&vertShaderFile);
	char* vertShaderBytecode = (char*)gpMemorySystem->MemAllocate(sizeof(char) * vertShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	
	size_t vertShaderCodeSize;
	if (!gpFileSystem->ReadWholeBinaryFile(vertShaderBytecode, vertShaderFileSize, &vertShaderCodeSize, &vertShaderFile))
	{
		LOG_ERROR("Failed to read shader file: '%s'", vertShaderFile.path);
		return false;
	}

	VkShaderModuleCreateInfo vertShaderCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertShaderCreateInfo.codeSize = vertShaderCodeSize;
	vertShaderCreateInfo.pCode = (unsigned int*)vertShaderBytecode;

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.m_LogicalDevice, &vertShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_VertModule)))
	{
		LOG_ERROR("Failed to create vertex shader module for '%s'", vertShaderFile.path);
		return false;
	}

	// Fragment shader module
	FileHandle_t fragShaderFile;
	if (!gpFileSystem->OpenFile("Assets/Shaders/frag.spv", FILEMODE_READ, true, &fragShaderFile))
	{
		LOG_ERROR("Failed to read fragment shader file");
		return false;
	}

	size_t fragShaderFileSize = gpFileSystem->FileSize(&fragShaderFile);
	char* fragShaderBytecode = (char*)gpMemorySystem->MemAllocate(sizeof(char) * fragShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);

	size_t fragShaderCodeSize;
	if (!gpFileSystem->ReadWholeBinaryFile(fragShaderBytecode, fragShaderFileSize, &fragShaderCodeSize, &fragShaderFile))
	{
		LOG_ERROR("Failed to read shader file: '%s'", fragShaderFile.path);
		return false;
	}

	VkShaderModuleCreateInfo fragShaderCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragShaderCreateInfo.codeSize = fragShaderCodeSize;
	fragShaderCreateInfo.pCode = (unsigned int*)fragShaderBytecode;

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.m_LogicalDevice, &fragShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_FragModule)))
	{
		LOG_ERROR("Failed to create fragment shader module for '%s'", fragShaderFile.path);
		return false;
	}

	// Release unnecessary resources
	gpMemorySystem->MemFree(fragShaderBytecode, sizeof(char) * fragShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	gpFileSystem->CloseFile(&fragShaderFile);

	gpMemorySystem->MemFree(vertShaderBytecode, sizeof(char) * vertShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	gpFileSystem->CloseFile(&vertShaderFile);

	// Create the pipeline object
	if (!pOutShader->CreatePipeline(pRenderingContext, pOutShader))
	{
		LOG_ERROR("Failed to create pipeline for Vulkan Shader.");
		return false;
	}

	// Delete unnecessary shader modules
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_FragModule, pRenderingContext->m_Allocator);
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_VertModule, pRenderingContext->m_Allocator);

	return true;
}

void VulkanShader::DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader)
{
	if (pShader)
	{
		if (pShader->m_Pipeline)
			vkDestroyPipeline(pRenderingContext->m_Device.m_LogicalDevice, pShader->m_Pipeline, pRenderingContext->m_Allocator);

		if (pShader->m_PipelineLayout)
			vkDestroyPipelineLayout(pRenderingContext->m_Device.m_LogicalDevice, pShader->m_PipelineLayout, pRenderingContext->m_Allocator);
	}
}

bool VulkanShader::CreatePipeline(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader)
{
	// Shader modules
	VkPipelineShaderStageCreateInfo shaderStageInfos[2]{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].pName = "main";
	shaderStageInfos[0].module = pOutShader->m_VertModule;


	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].pName = "main";
	shaderStageInfos[1].module = pOutShader->m_FragModule;

	// Vertex Assembly
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// Vertex input attributes
	// TODO: Make configurable
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex3D);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;

	VkVertexInputAttributeDescription attributeDescriptions[2]{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = 0;

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex3D, Color);

	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)pRenderingContext->m_Swapchain.m_SwapchainExtent.width;
	viewport.height = (float)pRenderingContext->m_Swapchain.m_SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = pRenderingContext->m_Swapchain.m_SwapchainExtent;

	VkPipelineViewportStateCreateInfo viewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportInfo.viewportCount = 1;
	viewportInfo.scissorCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.pScissors = &scissor;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL; // SET THIS TO MODE_LINE for wireframe rendering
	rasterizationInfo.lineWidth = 1.0f;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampleInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	// TODO: Depth/Stencil testing

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// Dynamic state
	VkDynamicState dynamicStates[2];
	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &pRenderingContext->m_DescriptorSetLayout;
	if (vkCreatePipelineLayout(pRenderingContext->m_Device.m_LogicalDevice, &pipelineLayoutInfo, pRenderingContext->m_Allocator, &pOutShader->m_PipelineLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Vulkan Pipeline Layout");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stageCount = 2; // TODO: Hardcoded
	pipelineCreateInfo.pStages = shaderStageInfos;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &vertexAssemblyCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlending;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = pOutShader->m_PipelineLayout;
	pipelineCreateInfo.renderPass = pRenderingContext->m_MainRenderpass.m_Handle;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (!VK_CHECK(vkCreateGraphicsPipelines(pRenderingContext->m_Device.m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_Pipeline)))
	{
		return false;
	}

	return true;
}

bool VulkanShader::Bind(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer) const
{
	vkCmdBindPipeline(*pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	return true;
}

