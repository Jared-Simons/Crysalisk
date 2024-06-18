#include "VulkanBackend.h"

#include "Core/Logging.h"

#include "Systems/IFileSystem.h"


bool VulkanShader::CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader)
{
	// Create shader modules

	// Vertex Shader Module
	FileHandle_t vertShaderHandle;

	if (!gpFileSystem->OpenFile("Assets/Shaders/vert.spv", FILEMODE_READ, true, &vertShaderHandle))
	{
		LOG_ERROR("Failed to create Vulkan shader.");
		return false;
	}
	size_t vertShaderFileSize = gpFileSystem->FileSize(&vertShaderHandle);
	char* vertShaderBytecode = new char[vertShaderFileSize];
	size_t vertShaderCodeSize;

	if (!gpFileSystem->ReadWholeBinaryFile(vertShaderBytecode, vertShaderFileSize, &vertShaderCodeSize, &vertShaderHandle))
	{
		LOG_ERROR("Failed to read shader file: '%s'", vertShaderHandle.path);
		return false;
	}

	VkShaderModuleCreateInfo vertShaderModule{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertShaderModule.codeSize = vertShaderCodeSize;
	vertShaderModule.pCode = (unsigned int*)vertShaderBytecode;

	if (vkCreateShaderModule(pRenderingContext->GetDevice().GetLogicalDevice(), &vertShaderModule, nullptr, &pOutShader->m_VertModule) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create vertex shader module for '%s'", vertShaderHandle.path);
	}

	// Fragment Shader Module
	FileHandle_t fragShaderHandle;

	if (!gpFileSystem->OpenFile("Assets/Shaders/frag.spv", FILEMODE_READ, true, &fragShaderHandle))
	{
		LOG_ERROR("Failed to create Vulkan shader.");
		return false;
	}
	size_t fragShaderFileSize = gpFileSystem->FileSize(&fragShaderHandle);
	char* fragShaderBytecode = new char[vertShaderFileSize];
	size_t fragShaderCodeSize;

	if (!gpFileSystem->ReadWholeBinaryFile(fragShaderBytecode, fragShaderFileSize, &fragShaderCodeSize, &fragShaderHandle))
	{
		LOG_ERROR("Failed to read shader file: '%s'", fragShaderHandle.path);
		return false;
	}

	VkShaderModuleCreateInfo fragShaderModule{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragShaderModule.codeSize = fragShaderCodeSize;
	fragShaderModule.pCode = (unsigned int*)fragShaderBytecode;

	if (vkCreateShaderModule(pRenderingContext->GetDevice().GetLogicalDevice(), &fragShaderModule, nullptr, &pOutShader->m_FragModule) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create fragment shader module for '%s'", fragShaderHandle.path);
	}

	// Release unnecessary resources.
	delete[] fragShaderBytecode;
	gpFileSystem->CloseFile(&fragShaderHandle);

	delete[] vertShaderBytecode;
	gpFileSystem->CloseFile(&vertShaderHandle);

	// Create the pipeline object

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

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport
	VkViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)pRenderingContext->GetSwapchain().GetExtent().width;
	viewport.height = (float)pRenderingContext->GetSwapchain().GetExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = pRenderingContext->GetSwapchain().GetExtent();

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
	rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampleInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	// TODO: Depth/Stencil testing

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
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


	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	// TODO: Custom allocator
	if (vkCreatePipelineLayout(pRenderingContext->GetDevice().GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pOutShader->m_PipelineLayout) != VK_SUCCESS)
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
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = pOutShader->m_PipelineLayout;
	pipelineCreateInfo.renderPass = pRenderingContext->GetRenderpass().GetHandle();
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	// TODO: Custom allocator
	if (vkCreateGraphicsPipelines(pRenderingContext->GetDevice().GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pOutShader->m_Pipeline) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Vulkan Graphics Pipeline");
		return false;
	}

	// Delete unnecessary shader modules
	// TODO: Custom allocator
	vkDestroyShaderModule(pRenderingContext->GetDevice().GetLogicalDevice(), pOutShader->m_FragModule, nullptr);
	vkDestroyShaderModule(pRenderingContext->GetDevice().GetLogicalDevice(), pOutShader->m_VertModule, nullptr);

	LOG_INFO("Vulkan Pipeline Created");

	return true;
}

void VulkanShader::DestroyVulkanShader(CVulkanRenderer* pRenderingContext)
{
	// TODO: Custom allocator
	vkDestroyPipeline(pRenderingContext->GetDevice().GetLogicalDevice(), m_Pipeline, nullptr);

	vkDestroyPipelineLayout(pRenderingContext->GetDevice().GetLogicalDevice(), m_PipelineLayout, nullptr);
}

bool VulkanShader::BindShader(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer)
{
	vkCmdBindPipeline(*pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	return true;
}

