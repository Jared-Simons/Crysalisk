//=============================================================================
// Author: Jared Simons
// Date Created: 7/2/2024
// 
// Purpose: Methods and utilities for handling Vulkan shader data.
//=============================================================================

#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"
#include "Core/Memory.h"
#include "Platform/FileSystem.h"
#include "Resources/ResourceTypes.h"
#include "Renderer/RendererTypes.h"

#include <Math/VertexTypes.h>
#include <String_Utils.h>


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
	if (!FileSystem::OpenFile("Assets/Shaders/vert.spv", FILEMODE_READ, true, &vertShaderFile))
	{
		LOG_ERROR("Failed to read vertex shader file");
		return false;
	}

	u64 vertShaderFileSize = FileSystem::FileSize(&vertShaderFile);
	byte* vertShaderBytecode = reinterpret_cast<byte*>(Memory::Allocate(sizeof(byte) * vertShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY));
	
	u64 vertShaderCodeSize;
	if (!FileSystem::ReadWholeBinaryFile(vertShaderBytecode, vertShaderFileSize, &vertShaderCodeSize, &vertShaderFile))
	{
		LOG_ERROR("Failed to read shader file: '%s'", vertShaderFile.path);
		return false;
	}

	VkShaderModuleCreateInfo vertShaderCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertShaderCreateInfo.codeSize = vertShaderCodeSize;
	vertShaderCreateInfo.pCode = reinterpret_cast<u32*>(vertShaderBytecode);

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.GetLogicalDevice(), &vertShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_VertModule)))
	{
		LOG_ERROR("Failed to create vertex shader module for '%s'", vertShaderFile.path);
		return false;
	}

	// Fragment shader module
	FileHandle_t fragShaderFile;
	if (!FileSystem::OpenFile("Assets/Shaders/frag.spv", FILEMODE_READ, true, &fragShaderFile))
	{
		LOG_ERROR("Failed to read fragment shader file");
		return false;
	}

	u64 fragShaderFileSize = FileSystem::FileSize(&fragShaderFile);
	byte* fragShaderBytecode = reinterpret_cast<byte*>(Memory::Allocate(sizeof(byte) * fragShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY));

	u64 fragShaderCodeSize;
	if (!FileSystem::ReadWholeBinaryFile(fragShaderBytecode, fragShaderFileSize, &fragShaderCodeSize, &fragShaderFile))
	{
		LOG_ERROR("Failed to read shader file: '%s'", fragShaderFile.path);
		return false;
	}

	VkShaderModuleCreateInfo fragShaderCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragShaderCreateInfo.codeSize = fragShaderCodeSize;
	fragShaderCreateInfo.pCode = reinterpret_cast<u32*>(fragShaderBytecode);

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.GetLogicalDevice(), &fragShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_FragModule)))
	{
		LOG_ERROR("Failed to create fragment shader module for '%s'", fragShaderFile.path);
		return false;
	}

	// Release unnecessary resources
	Memory::Free(fragShaderBytecode, sizeof(byte) * fragShaderCodeSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	FileSystem::CloseFile(&fragShaderFile);

	Memory::Free(vertShaderBytecode, sizeof(byte) * vertShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	FileSystem::CloseFile(&vertShaderFile);

	if (!pOutShader->CreateDescriptorResources(pRenderingContext))
	{
		LOG_ERROR("Failed to create descriptor resources for Vulkan Shader.");
		return false;
	}

	// Create the pipeline object
	if (!pOutShader->CreatePipeline(pRenderingContext, pOutShader))
	{
		LOG_ERROR("Failed to create pipeline for Vulkan Shader.");
		return false;
	}

	// Create uniform buffer.
	VkDeviceSize uniformBufferSize = sizeof(GlobalUniformObject) + ((sizeof(InstanceUniformObject) * VULKAN_MAX_INSTANCE_COUNT)); // Enough space for one uniform buffer and many instances.
	VulkanBuffer::CreateBuffer(pRenderingContext, uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pOutShader->m_UniformBuffer);
	vkMapMemory(pRenderingContext->m_Device.GetLogicalDevice(), pOutShader->m_UniformBuffer.GetMemory(), 0, VK_WHOLE_SIZE, 0, &pOutShader->m_UniformBufferMappedMemory);

	// Release unnecessary shader modules
	vkDestroyShaderModule(pRenderingContext->m_Device.GetLogicalDevice(), pOutShader->m_FragModule, pRenderingContext->m_Allocator);
	vkDestroyShaderModule(pRenderingContext->m_Device.GetLogicalDevice(), pOutShader->m_VertModule, pRenderingContext->m_Allocator);

	pOutShader->m_InstanceDescriptorOffset = 1;

	// TEMP: Create a texture to use as the global texture for now.
	VulkanImage::CreateImage(pRenderingContext, 16, 16, 4, nullptr, true, &pOutShader->m_GlobalTexture);
	pOutShader->m_GlobalTexture.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	// END TEMP

	return true;
}

void VulkanShader::DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader)
{
	if (pShader)
	{
		VulkanImage::DestroyImage(pRenderingContext, &pShader->m_GlobalTexture);

		VulkanBuffer::DestroyBuffer(pRenderingContext, &pShader->m_UniformBuffer);

		vkDestroyDescriptorPool(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_GlobalDescriptorPool, pRenderingContext->m_Allocator);
		vkDestroyDescriptorPool(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_InstanceDescriptorPool, pRenderingContext->m_Allocator);

		vkDestroyDescriptorSetLayout(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_GlobalDescriptorSetLayout, pRenderingContext->m_Allocator);
		vkDestroyDescriptorSetLayout(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_InstanceDescriptorSetLayout, pRenderingContext->m_Allocator);

		if (pShader->m_Pipeline)
			vkDestroyPipeline(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_Pipeline, pRenderingContext->m_Allocator);

		if (pShader->m_PipelineLayout)
			vkDestroyPipelineLayout(pRenderingContext->m_Device.GetLogicalDevice(), pShader->m_PipelineLayout, pRenderingContext->m_Allocator);
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

	VkVertexInputAttributeDescription attributeDescriptions[3]{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex3D, Pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex3D, Color);
	
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex3D, TexCoord);

	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<f32>(pRenderingContext->m_Swapchain.GetExtent().width);
	viewport.height = static_cast<f32>(pRenderingContext->m_Swapchain.GetExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = pRenderingContext->m_Swapchain.GetExtent();

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
	VkDynamicState dynamicStates[2]{};
	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Push constants
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(Matrix4x4);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	VkDescriptorSetLayout pDescriptorLayouts[2] = { pOutShader->m_GlobalDescriptorSetLayout, pOutShader->m_InstanceDescriptorSetLayout };
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = pDescriptorLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

	if (vkCreatePipelineLayout(pRenderingContext->m_Device.GetLogicalDevice(), &pipelineLayoutInfo, pRenderingContext->m_Allocator, &pOutShader->m_PipelineLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Vulkan Pipeline Layout");
		return false;
	}

	LOG_INFO("Vulkan Shader Pipeline created");

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
	pipelineCreateInfo.renderPass = pRenderingContext->m_MainRenderpass.GetHandle();
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (!VK_CHECK(vkCreateGraphicsPipelines(pRenderingContext->m_Device.GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_Pipeline)))
	{
		return false;
	}

	return true;
}

bool VulkanShader::Use(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer, u32 imageIndex) const
{
	vkCmdBindPipeline(*pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	return true;
}

void VulkanShader::UpdateUniforms(CVulkanRenderer* pRenderingContext, u32 imageIndex, struct Camera s_Camera, f32 deltaTime)
{
	static f32 startTime = static_cast<f32>(Platform::GetAbsoluteTime());
	f32 currentTime = static_cast<f32>(Platform::GetAbsoluteTime());

	f32 time = currentTime - startTime;

	UniformBufferObject ubo{};
	ubo.model = Matrix4x4(1.0f);
	ubo.model = Matrix4x4::Translate(ubo.model, Vector3D(0.0f, 0.0f, 0.0f));
	ubo.model = Matrix4x4::Rotate(Matrix4x4(1.0f), time * M_DEG2RAD(45.0f), Vector3D(0.0f, 1.0f, 0.0f));
	ubo.view = s_Camera.GetViewMatrix();
	//ubo.view = Matrix4x4::LookAt(Vector3D(0.0f, 0.0f, 3.0f), Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));

	ubo.projection = Matrix4x4::Perspective(M_DEG2RAD(45.0f), pRenderingContext->m_Swapchain.GetExtent().width / (f32)pRenderingContext->m_Swapchain.GetExtent().height, 0.1f, 100.0f);
	ubo.projection.data[5] *= -1; // Flip the Y coordinate
}

bool VulkanShader::CreateDescriptorResources(CVulkanRenderer* pRenderingContext)
{
	// Global Descriptor Pool
	VkDescriptorPoolSize globalPoolSizes[2]{};

	globalPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalPoolSizes[0].descriptorCount = 1;

	globalPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	globalPoolSizes[1].descriptorCount = VULKAN_MAX_GLOBAL_TEXTURES;

	VkDescriptorPoolCreateInfo globalPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	globalPoolInfo.maxSets = 3; // One per frame in flight
	globalPoolInfo.poolSizeCount = 2;
	globalPoolInfo.pPoolSizes = globalPoolSizes;

	if (!VK_CHECK(vkCreateDescriptorPool(pRenderingContext->m_Device.GetLogicalDevice(), &globalPoolInfo, pRenderingContext->m_Allocator, &m_GlobalDescriptorPool)))
	{
		LOG_ERROR("Failed to create global descriptor pool for Vulkan shader.");
		return false;
	}

	// Global Descriptor Set Layout
	VkDescriptorSetLayoutBinding globalLayoutBindings[2]{};

	// Binding 0 - Uniform Buffer.
	globalLayoutBindings[0].binding = 0;
	globalLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalLayoutBindings[0].descriptorCount = 1;
	globalLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	globalLayoutBindings[0].pImmutableSamplers = nullptr;
	
	// Binding 1 - Image Sampler.
	globalLayoutBindings[1].binding = 1;
	globalLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	globalLayoutBindings[1].descriptorCount = 1;
	globalLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	globalLayoutBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo globalDescriptorSetLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	globalDescriptorSetLayoutInfo.pNext = nullptr;
	globalDescriptorSetLayoutInfo.flags = 0;
	globalDescriptorSetLayoutInfo.bindingCount = 2;
	globalDescriptorSetLayoutInfo.pBindings = globalLayoutBindings;

	if (!VK_CHECK(vkCreateDescriptorSetLayout(pRenderingContext->m_Device.GetLogicalDevice(), &globalDescriptorSetLayoutInfo, pRenderingContext->m_Allocator, &m_GlobalDescriptorSetLayout)))
	{
		LOG_ERROR("Failed to create global descriptor set layout for Vulkan shader.");
		return false;
	}

	// Instance Descriptor Pool
	VkDescriptorPoolSize instancePoolSizes[2]{};

	instancePoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	instancePoolSizes[0].descriptorCount = VULKAN_MAX_INSTANCE_COUNT;

	instancePoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	instancePoolSizes[1].descriptorCount = VULKAN_MAX_INSTANCE_COUNT * VULKAN_SHADER_SAMPLER_COUNT;

	VkDescriptorPoolCreateInfo instancePoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	instancePoolInfo.maxSets = VULKAN_MAX_INSTANCE_COUNT;
	instancePoolInfo.poolSizeCount = 2;
	instancePoolInfo.pPoolSizes = instancePoolSizes;

	if (!VK_CHECK(vkCreateDescriptorPool(pRenderingContext->m_Device.GetLogicalDevice(), &instancePoolInfo, pRenderingContext->m_Allocator, &m_InstanceDescriptorPool)))
	{
		LOG_ERROR("Failed to create instance descriptor pool for Vulkan shader.");
		return false;
	}

	// Instance Descriptor Set Layout
	VkDescriptorSetLayoutBinding instanceLayoutBindings[2]{};

	// Binding 0 - Uniform Buffer.
	instanceLayoutBindings[0].binding = 0;
	instanceLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	instanceLayoutBindings[0].descriptorCount = 1;
	instanceLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	instanceLayoutBindings[0].pImmutableSamplers = nullptr;

	// Binding 1 - Image Sampler.
	instanceLayoutBindings[1].binding = 1;
	instanceLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	instanceLayoutBindings[1].descriptorCount = 1;
	instanceLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	instanceLayoutBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo instanceDescriptorSetLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	instanceDescriptorSetLayoutInfo.pNext = nullptr;
	instanceDescriptorSetLayoutInfo.flags = 0;
	instanceDescriptorSetLayoutInfo.bindingCount = 2;
	instanceDescriptorSetLayoutInfo.pBindings = instanceLayoutBindings;

	if (!VK_CHECK(vkCreateDescriptorSetLayout(pRenderingContext->m_Device.GetLogicalDevice(), &instanceDescriptorSetLayoutInfo, pRenderingContext->m_Allocator, &m_InstanceDescriptorSetLayout)))
	{
		LOG_ERROR("Failed to create global descriptor set layout for Vulkan shader.");
		return false;
	}

	// Allocate Global Descriptor Sets.

	for (u32 i = 0; i < 3; i++)
	{
		VkDescriptorSetAllocateInfo globalDescriptorAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

		globalDescriptorAllocInfo.pNext = nullptr;
		globalDescriptorAllocInfo.descriptorPool = m_GlobalDescriptorPool;
		globalDescriptorAllocInfo.descriptorSetCount = 1;
		globalDescriptorAllocInfo.pSetLayouts = &m_GlobalDescriptorSetLayout;

		if (!VK_CHECK(vkAllocateDescriptorSets(pRenderingContext->m_Device.GetLogicalDevice(), &globalDescriptorAllocInfo, &m_GlobalDescriptorSets.descriptorSets[i])))
		{
			LOG_ERROR("Failed to allocate global descriptor sets for Vulkan shader.");
			return false;
		}

		// Invalidate our generation so that we get updated asap.
		m_GlobalDescriptorSets.generation[i] = INVALID_ID_U8;
	}

	return true;
}

bool VulkanShader::AcquireInstanceResources(CVulkanRenderer* pRenderingContext, Texture& pTexture)
{
	// Allocate our descriptor sets.
	for (u32 imageIndex = 0; imageIndex < 3; imageIndex++)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocInfo.pNext = nullptr;
		descriptorSetAllocInfo.descriptorPool = m_InstanceDescriptorPool;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &m_InstanceDescriptorSetLayout;

		if (!VK_CHECK(vkAllocateDescriptorSets(pRenderingContext->m_Device.GetLogicalDevice(), &descriptorSetAllocInfo, &m_InstanceDescriptorSets[m_InstanceDescriptorOffset].descriptorSets[imageIndex])))
		{
			LOG_ERROR("Failed to allocate instance descriptors for texture: %s", pTexture.m_TextureName);
			return false;
		}

		// Invalidate our generation so that we get updated asap.
		m_InstanceDescriptorSets[m_InstanceDescriptorOffset].generation[imageIndex] = INVALID_ID_U8;
	}

	VulkanImageData_t* pInternalData = reinterpret_cast<VulkanImageData_t*>(pTexture.m_internalData);

	pInternalData->offset = m_InstanceDescriptorOffset;
	m_InstanceDescriptorOffset++;

	return true;
}

void VulkanShader::BindGlobals(VkCommandBuffer commandBuffer, u32 imageIndex)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_GlobalDescriptorSets.descriptorSets[imageIndex], 0, nullptr);

	m_CurrentlyBoundInstance = 0; // The Global uniform is the first instance in the uniform buffer
}

void VulkanShader::ApplyGlobals(CVulkanRenderer* pRenderingContext, VkCommandBuffer commandBuffer, u32 imageIndex, Matrix4x4 projection, Matrix4x4 view)
{
	// Update descriptors if necessary.
	if (m_GlobalDescriptorSets.generation[imageIndex] == INVALID_ID_U8)
	{
		VkWriteDescriptorSet descriptorWrites[2]{};

		// Binding 0 - Uniform Buffer
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffer.GetHandle();
		bufferInfo.offset = 0; // Globals are at the front of the buffer
		bufferInfo.range = sizeof(GlobalUniformObject);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].pNext = nullptr;
		descriptorWrites[0].dstSet = m_GlobalDescriptorSets.descriptorSets[imageIndex];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// Binding 1 - Image Sampler
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_GlobalTexture.GetImageView();
		imageInfo.sampler = m_GlobalTexture.GetSampler();

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].pNext = nullptr;
		descriptorWrites[1].dstSet = m_GlobalDescriptorSets.descriptorSets[imageIndex];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(pRenderingContext->m_Device.GetLogicalDevice(), 2, descriptorWrites, 0, nullptr);

		if (m_GlobalDescriptorSets.generation[imageIndex] == INVALID_ID_U8)
			m_GlobalDescriptorSets.generation[imageIndex] = 0;
		else
			m_GlobalDescriptorSets.generation[imageIndex]++;
	}

	// Set uniforms.
	GlobalUniformObject GUO{ view, projection };
	Memory::MemCopy(m_UniformBufferMappedMemory, &GUO, sizeof(GUO)); // Copy to the front of the buffer (as that's where the globals live)
}

void VulkanShader::BindInstance(VkCommandBuffer commandBuffer, u32 imageIndex, Texture& pTexture)
{
	u32 uniformOffset = (reinterpret_cast<VulkanImageData_t*>(pTexture.m_internalData)->offset);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &m_InstanceDescriptorSets[uniformOffset].descriptorSets[imageIndex], 0, nullptr);
	m_CurrentlyBoundInstance = uniformOffset;
}

void VulkanShader::ApplyInstance(CVulkanRenderer* pRenderingContext, VkCommandBuffer commandBuffer, u32 imageIndex, Texture& pTexture)
{
	VulkanImageData_t* pInternalTextureData = reinterpret_cast<VulkanImageData_t*>(pTexture.m_internalData);

	// Update descriptors if necessary.
	if (m_InstanceDescriptorSets[pInternalTextureData->offset].generation[imageIndex] == INVALID_ID_U8)
	{
		VkWriteDescriptorSet descriptorWrites[2]{};

		// Binding 0 - Uniform Buffer
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffer.GetHandle();
		bufferInfo.offset = sizeof(GlobalUniformObject) + ((sizeof(InstanceUniformObject) * pInternalTextureData->offset));
		bufferInfo.range = sizeof(InstanceUniformObject);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].pNext = nullptr;
		descriptorWrites[0].dstSet = m_InstanceDescriptorSets[pInternalTextureData->offset].descriptorSets[imageIndex];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// Binding 1 - Image Sampler
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = pInternalTextureData->internalImage.GetImageView();
		imageInfo.sampler = pInternalTextureData->internalImage.GetSampler();

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].pNext = nullptr;
		descriptorWrites[1].dstSet = m_InstanceDescriptorSets[pInternalTextureData->offset].descriptorSets[imageIndex];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(pRenderingContext->m_Device.GetLogicalDevice(), 2, descriptorWrites, 0, nullptr);

		// TODO: This is slow!
		if (!StringCompare(pTexture.m_TextureName, "DefaultDiffuse"))
		{
			if (m_InstanceDescriptorSets[pInternalTextureData->offset].generation[imageIndex] == INVALID_ID_U8)
				m_InstanceDescriptorSets[pInternalTextureData->offset].generation[imageIndex] = 0;
			else
				m_InstanceDescriptorSets[pInternalTextureData->offset].generation[imageIndex]++;
		}
	}

	// Set uniforms.
	InstanceUniformObject instanceUniforms;
	instanceUniforms.color = Vector4D(1.0f, 1.0f, 1.0f, 1.0f); // White
	
	void* instanceUniformAddress = reinterpret_cast<byte*>(m_UniformBufferMappedMemory) + sizeof(GlobalUniformObject);
	instanceUniformAddress = reinterpret_cast<byte*>(instanceUniformAddress) + (sizeof(InstanceUniformObject) * pInternalTextureData->offset);

	Memory::MemCopy(instanceUniformAddress, &instanceUniforms, sizeof(InstanceUniformObject));
}

void VulkanShader::ApplyModel(VkCommandBuffer commandBuffer, Matrix4x4 model) const
{
	vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4x4), &model);
}