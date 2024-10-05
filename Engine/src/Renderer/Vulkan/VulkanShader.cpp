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
#include "Resources/ResourceTypes.h"

#include <Math/VertexTypes.h>


bool VulkanShader::CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pRenderpass, VulkanShader* pOutShader)
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

	// Create Descriptor Resources
	if (!pOutShader->CreateDescriptorResources(pRenderingContext))
	{
		LOG_ERROR("Failed to create descriptor resources for Vulkan Shader.");
		return false;
	}

	// Create the pipeline object
	if (!pOutShader->CreatePipeline(pRenderingContext, pRenderpass, false))
	{
		LOG_ERROR("Failed to create pipeline for Vulkan Shader.");
		return false;
	}

	// Create buffer resources
	VkDeviceSize globalUBOSize = sizeof(UniformBufferObject);
	VulkanBuffer::CreateBuffer(pRenderingContext, globalUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pOutShader->m_GlobalUBO);

	VkDeviceSize instanceUBOSize = sizeof(UniformBufferObject) * MAX_VULKAN_INSTANCE_COUNT;
	VulkanBuffer::CreateBuffer(pRenderingContext, instanceUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pOutShader->m_InstanceUniformBuffer);

	// Release unnecessary resources
	gpMemorySystem->MemFree(fragShaderBytecode, sizeof(char) * fragShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	gpFileSystem->CloseFile(&fragShaderFile);

	gpMemorySystem->MemFree(vertShaderBytecode, sizeof(char) * vertShaderFileSize, eMemAllocCategories::MEM_CATEGORY_ARRAY);
	gpFileSystem->CloseFile(&vertShaderFile);

	// Release unnecessary shader modules
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_FragModule, pRenderingContext->m_Allocator);
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_VertModule, pRenderingContext->m_Allocator);

	// Invalidate our currently-bound index.
	pOutShader->m_CurrentlyBoundInstance = -1; // TODO: Invalid ID

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

void VulkanShader::Use(CVulkanRenderer* pRenderingContext, VkCommandBuffer pCommandBuffer, uint32_t imageIndex)
{
	vkCmdBindPipeline(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

void VulkanShader::BindInstance(size_t instanceID)
{
	if (m_CurrentlyBoundInstance == instanceID)
		return;

	m_CurrentlyBoundInstance = instanceID;
}

void VulkanShader::ApplyGlobals(CVulkanRenderer* pRenderingContext, VkCommandBuffer pCommandBuffer, uint32_t imageIndex)
{
	if (m_GlobalDescriptorSets.generation == INVALID_ID_U8)
	{
		for (int index = 0; index < 3; index++)
		{
			VkWriteDescriptorSet descriptorWrites[1];

			// Binding 0 - Uniform Buffer
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = m_GlobalUBO.m_Handle;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].pNext = nullptr;
			descriptorWrites[0].dstSet = m_GlobalDescriptorSets.descriptorSets[index];
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorCount = 1; 
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, 1, descriptorWrites, 0, nullptr);
		}

		m_GlobalDescriptorSets.generation = 0;
	}

	vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_GlobalDescriptorSets.descriptorSets[imageIndex], 0, nullptr);
}

void VulkanShader::ApplyInstance(CVulkanRenderer* pRenderingContext, VkCommandBuffer pCommandBuffer, uint32_t imageIndex, VulkanTextureData_t* pTextureData)
{
	if (m_InstanceDescriptors[m_CurrentlyBoundInstance].generation == INVALID_ID_U8)
	{
		for (int index = 0; index < 3; index++)
		{
			VkWriteDescriptorSet descriptorWrites[2];

			// Binding 0 - Uniform Buffer
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = m_InstanceUniformBuffer.m_Handle;
			bufferInfo.offset = pTextureData->m_descriptorIndex * sizeof(UniformBufferObject);
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].pNext = nullptr;
			descriptorWrites[0].dstSet = m_InstanceDescriptors[m_CurrentlyBoundInstance].descriptorSets[index];
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			// Binding 1 - Image Sampler
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = pTextureData->m_Image.m_ImageView;
			imageInfo.sampler = pTextureData->m_Image.m_ImageSampler;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].pNext = nullptr;
			descriptorWrites[1].dstSet = m_InstanceDescriptors[m_CurrentlyBoundInstance].descriptorSets[index];
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, 2, descriptorWrites, 0, nullptr);
		}

		m_InstanceDescriptors[m_CurrentlyBoundInstance].generation = 0;
	}

	vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &m_InstanceDescriptors[m_CurrentlyBoundInstance].descriptorSets[imageIndex], 0, nullptr);
}

bool VulkanShader::AcquireInstanceResources(CVulkanRenderer* pRenderingContext, unsigned int* pOutDescriptorIndex)
{
	if (m_InstanceDescriptorIndex == MAX_VULKAN_INSTANCE_COUNT)
	{
		// We're out of descriptors, boot out.
		LOG_ERROR("Vulkan shader has ran out of instance descriptor sets to allocate! Instance resource acquisition failed!");
		return false;
	}

	for (int i = 0; i < 3; i++)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocInfo.pNext = nullptr;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.descriptorPool = m_InstanceDescriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &m_InstanceDescriptorSetLayout;

		if (!VK_CHECK(vkAllocateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, &descriptorSetAllocInfo, &m_InstanceDescriptors[m_InstanceDescriptorIndex].descriptorSets[i])))
		{
			LOG_ERROR("Failed to allocate descriptor sets for instance resources.");
			return false;
		}
	}

	// Invalidate the descriptor's generation so it gets updated asap.
	m_InstanceDescriptors[m_InstanceDescriptorIndex].generation = INVALID_ID_U8;

	*pOutDescriptorIndex = m_InstanceDescriptorIndex;
	m_InstanceDescriptorIndex++;

	return true;
}

bool VulkanShader::CreateDescriptorResources(CVulkanRenderer* pRenderingContext)
{// Global descriptors
	VkDescriptorSetLayoutBinding globalDescriptorBindings[2];
	// Binding 0 - Uniform Buffer
	globalDescriptorBindings[0].binding = 0;
	globalDescriptorBindings[0].descriptorCount = 1;
	globalDescriptorBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalDescriptorBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	globalDescriptorBindings[0].pImmutableSamplers = nullptr;

	// NOTE: Currently only using Global UBO
	/*// Binding 1 - Image Sampler
	globalDescriptorBindings[1].binding = 1;
	globalDescriptorBindings[1].descriptorCount = 1;
	globalDescriptorBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	globalDescriptorBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	globalDescriptorBindings[1].pImmutableSamplers = nullptr;*/

	VkDescriptorSetLayoutCreateInfo globalLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	globalLayoutInfo.bindingCount = 1;
	globalLayoutInfo.pBindings = globalDescriptorBindings;

	if (!VK_CHECK(vkCreateDescriptorSetLayout(pRenderingContext->m_Device.m_LogicalDevice, &globalLayoutInfo, pRenderingContext->m_Allocator, &m_GlobalDescriptorSetLayout)))
	{
		LOG_ERROR("Shader failed to create global descriptor set layout!");
		return false;
	}

	// Global descriptor pool
	VkDescriptorPoolSize globalPoolSizes[2];

	globalPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalPoolSizes[0].descriptorCount = 1;

	//globalPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//globalPoolSizes[1].descriptorCount = 1; // * MAX_GLOBAL_TEXTURES

	VkDescriptorPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = globalPoolSizes;
	poolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

	if (!VK_CHECK(vkCreateDescriptorPool(pRenderingContext->m_Device.m_LogicalDevice, &poolCreateInfo, pRenderingContext->m_Allocator, &m_GlobalDescriptorPool)))
	{
		LOG_ERROR("Failed to create global descriptor pool");
		return false;
	}

	// Allocate our global descriptor sets.
	for (int i = 0; i < 3; i++)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocInfo.pNext = nullptr;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.descriptorPool = m_GlobalDescriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &m_GlobalDescriptorSetLayout;

		if (!VK_CHECK(vkAllocateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, &descriptorSetAllocInfo, &m_GlobalDescriptorSets.descriptorSets[i])))
		{
			LOG_ERROR("Failed to allocate descriptor sets for global uniforms.");
			return false;
		}
	}

	// Invalidate the descriptor's generation so it gets updated asap.
	m_GlobalDescriptorSets.generation = INVALID_ID_U8;

	// Instance Descriptor Resources
	VkDescriptorSetLayoutBinding instanceDescriptorBindings[2];
	// Binding 0 - Uniform Buffer
	instanceDescriptorBindings[0].binding = 0;
	instanceDescriptorBindings[0].descriptorCount = 1;
	instanceDescriptorBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	instanceDescriptorBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	instanceDescriptorBindings[0].pImmutableSamplers = nullptr;

	// Binding 1 - Image Sampler
	instanceDescriptorBindings[1].binding = 1;
	instanceDescriptorBindings[1].descriptorCount = 1;
	instanceDescriptorBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	instanceDescriptorBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	instanceDescriptorBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo instanceLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	instanceLayoutInfo.bindingCount = 2;
	instanceLayoutInfo.pBindings = instanceDescriptorBindings;

	if (!VK_CHECK(vkCreateDescriptorSetLayout(pRenderingContext->m_Device.m_LogicalDevice, &instanceLayoutInfo, pRenderingContext->m_Allocator, &m_InstanceDescriptorSetLayout)))
	{
		LOG_ERROR("Shader failed to create instance descriptor set layout!");
		return false;
	}

	// Instance descriptor pool
	VkDescriptorPoolSize instancelPoolSizes[2];

	instancelPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	instancelPoolSizes[0].descriptorCount = MAX_VULKAN_INSTANCE_COUNT;

	instancelPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	instancelPoolSizes[1].descriptorCount = MAX_VULKAN_INSTANCE_COUNT; // * MAX_GLOBAL_TEXTURES

	VkDescriptorPoolCreateInfo instancePoolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	instancePoolCreateInfo.poolSizeCount = 2;
	instancePoolCreateInfo.pPoolSizes = instancelPoolSizes;
	instancePoolCreateInfo.maxSets = MAX_VULKAN_INSTANCE_COUNT;

	if (!VK_CHECK(vkCreateDescriptorPool(pRenderingContext->m_Device.m_LogicalDevice, &instancePoolCreateInfo, pRenderingContext->m_Allocator, &m_InstanceDescriptorPool)))
	{
		LOG_ERROR("Failed to create instance descriptor pool");
		return false;
	}

	// NOTE: We do not allocate instance level descriptor sets here, rather they get allocated on resource acquisition.

	// Set our running instance descriptor index.
	m_InstanceDescriptorIndex = 0;

	return true;
}

bool VulkanShader::CreatePipeline(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pRenderpass, bool bIsWireframe)
{
	// Shader stages.
	VkPipelineShaderStageCreateInfo shaderStages[2];

	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].pNext = nullptr;
	shaderStages[0].flags = 0;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = m_VertModule;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].pNext = nullptr;
	shaderStages[1].flags = 0;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = m_FragModule;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = nullptr;

	// Vertex input binding.
	VkVertexInputBindingDescription vertexInputBinding;
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(Vertex3D);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Vertex input attributes.
	VkVertexInputAttributeDescription vertexAttributes[3]; // 3 Attributes: position, color, texcoord

	vertexAttributes[0].location = 0;
	vertexAttributes[0].binding = 0;
	vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributes[0].offset = offsetof(Vertex3D, Pos);

	vertexAttributes[1].location = 1;
	vertexAttributes[1].binding = 0;
	vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributes[1].offset = offsetof(Vertex3D, Color);

	vertexAttributes[2].location = 2;
	vertexAttributes[2].binding = 0;
	vertexAttributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[2].offset = offsetof(Vertex3D, TexCoord);

	// Vertex input state.
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = 3;
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;

	// Input assembly state.
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.pNext = nullptr;
	inputAssemblyState.flags = 0;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	// Viewport/Scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(pRenderingContext->m_Swapchain.m_SwapchainExtent.width);
	viewport.height = static_cast<float>(pRenderingContext->m_Swapchain.m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = pRenderingContext->m_Swapchain.m_SwapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.pNext = nullptr;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = bIsWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.0f;
	rasterizationState.depthBiasClamp = 0.0f; 
	rasterizationState.depthBiasSlopeFactor = 0.0f;

	// Multisampling.
	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.pNext = nullptr;
	multisampleState.flags = 0;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // No multisampling for now.

	// Depth/Stencil state.
	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilState.pNext = nullptr;
	depthStencilState.flags = 0;

	// Color blend state.

	// Color state.
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.pNext = nullptr;
	colorBlendState.flags = 0;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;

	// Dynamic state
	// TODO: Implement

	// Pipeline Layout.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 2;
	VkDescriptorSetLayout descriptorSetLayouts[2] = { m_GlobalDescriptorSetLayout, m_InstanceDescriptorSetLayout };
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (!VK_CHECK(vkCreatePipelineLayout(pRenderingContext->m_Device.m_LogicalDevice, &pipelineLayoutInfo, pRenderingContext->m_Allocator, &m_PipelineLayout)))
	{
		LOG_ERROR("Shader failed to create pipeline layout");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = 2; // TODO: Hardcoded for now.
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizationState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = pRenderpass->m_Handle;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;

	if (!VK_CHECK(vkCreateGraphicsPipelines(pRenderingContext->m_Device.m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, pRenderingContext->m_Allocator, &m_Pipeline)))
	{
		LOG_ERROR("Failed to create graphics pipeline");
		return false;
	}

	return true;
}