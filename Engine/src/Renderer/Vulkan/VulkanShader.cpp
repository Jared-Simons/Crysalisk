//=============================================================================
// Author: Jared Simons
// Date Created: 7/2/2024
// 
// Purpose: Methods and utilities for handling Vulkan shader data.
//=============================================================================

#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Memory.h"
#include "Platform/FileSystem.h"
#include "Resources/ResourceTypes.h"

#include <Math/VertexTypes.h>


bool VulkanShader::CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader)
{
	if (!pOutShader)
	{
		LOG_ERROR("CreateVulkanShader() -> Requies a pointer to a valid VulkanShader object.");
		return false;
	}

	// TODO: This setup requires at least one texture to exist prior to the texture system's creation
	// This will need to be reworked
	{
		Texture dummyTexture;
		byte* tempTexturePixels[16 * 16 * 4];
		Memory::MemSet(tempTexturePixels, 255, 16 * 16 * 4);
		dummyTexture.m_Width = dummyTexture.m_Height = 16;
		dummyTexture.m_ChannelCount = 4;
		if (!pOutShader->AcquireInstanceResources(pRenderingContext, dummyTexture, tempTexturePixels))
		{
			LOG_ERROR("Failed to acquire instance resources for Vulkan Shader.");
			return false;
		}

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

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.m_LogicalDevice, &vertShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_VertModule)))
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

	if (!VK_CHECK(vkCreateShaderModule(pRenderingContext->m_Device.m_LogicalDevice, &fragShaderCreateInfo, pRenderingContext->m_Allocator, &pOutShader->m_FragModule)))
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

	// Release unnecessary shader modules
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_FragModule, pRenderingContext->m_Allocator);
	vkDestroyShaderModule(pRenderingContext->m_Device.m_LogicalDevice, pOutShader->m_VertModule, pRenderingContext->m_Allocator);

	return true;
}

void VulkanShader::DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader)
{
	if (pShader)
	{
		//VulkanImage::DestroyImage(pRenderingContext, &pShader->m_TextureImage);

		vkDestroyDescriptorPool(pRenderingContext->m_Device.m_LogicalDevice, pShader->m_DescriptorPool, pRenderingContext->m_Allocator);

		for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VulkanBuffer::DestroyBuffer(pRenderingContext, &pShader->m_UniformBuffers[i]);
		}

		vkDestroyDescriptorSetLayout(pRenderingContext->m_Device.m_LogicalDevice, pShader->m_DescriptorSetLayout, pRenderingContext->m_Allocator);

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
	viewport.width = static_cast<f32>(pRenderingContext->m_Swapchain.m_SwapchainExtent.width);
	viewport.height = static_cast<f32>(pRenderingContext->m_Swapchain.m_SwapchainExtent.height);
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
	pipelineLayoutInfo.pSetLayouts = &pOutShader->m_DescriptorSetLayout;
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

bool VulkanShader::Bind(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer, u32 imageIndex) const
{
	vkCmdBindDescriptorSets(*pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[imageIndex], 0, nullptr);
	vkCmdBindPipeline(*pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	return true;
}

void VulkanShader::UpdateUniforms(CVulkanRenderer* pRenderingContext, u32 imageIndex, struct Camera s_Camera, f32 deltaTime)
{
	static f64 startTime = Platform::GetAbsoluteTime();
	f64 currentTime = Platform::GetAbsoluteTime();

	f64 time = currentTime - startTime;

	UniformBufferObject ubo{};
	ubo.model = Matrix4x4(1.0f);
	ubo.model = Matrix4x4::Translate(ubo.model, Vector3D(0.0f, 0.0f, 0.0f));
	ubo.model = Matrix4x4::Rotate(Matrix4x4(1.0f), time * M_DEG2RAD(45.0f), Vector3D(0.0f, 1.0f, 0.0f));
	ubo.view = s_Camera.GetViewMatrix();
	//ubo.view = Matrix4x4::LookAt(Vector3D(0.0f, 0.0f, 3.0f), Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));

	ubo.projection = Matrix4x4::Perspective(M_DEG2RAD(45.0f), pRenderingContext->m_Swapchain.m_SwapchainExtent.width / (f32)pRenderingContext->m_Swapchain.m_SwapchainExtent.height, 0.1f, 100.0f);
	ubo.projection.data[1][1] *= -1; // Flip the Y coordinate

	memcpy(m_UniformBuffersMappedMemory[imageIndex], &ubo, sizeof(ubo));
}

bool VulkanShader::AcquireInstanceResources(CVulkanRenderer* pRenderingContext, Texture& pTexture, void* pPixelData) // TODO: MAKE CONFIGURABLE!!!
{
	// Uniform buffer
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_UniformBuffers.Resize(MAX_FRAMES_IN_FLIGHT);
	m_UniformBuffersMappedMemory.Resize(MAX_FRAMES_IN_FLIGHT);

	for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VulkanBuffer::CreateBuffer(pRenderingContext, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_UniformBuffers[i]);
		vkMapMemory(pRenderingContext->m_Device.m_LogicalDevice, m_UniformBuffers[i].m_Memory, 0, bufferSize, 0, &m_UniformBuffersMappedMemory[i]);
	}

	VkDeviceSize imageSize = pTexture.m_Width * pTexture.m_Height * 4;
	
	VulkanBuffer imageBuffer;
	VulkanBuffer::CreateBuffer(pRenderingContext, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer);

	void* data;
	vkMapMemory(pRenderingContext->m_Device.m_LogicalDevice, imageBuffer.m_Memory, 0, imageSize, 0, &data);
		Memory::MemCopy(data, pPixelData, static_cast<size_t>(imageSize));
	vkUnmapMemory(pRenderingContext->m_Device.m_LogicalDevice, imageBuffer.m_Memory);

	// Create Vulkan Resources for image
	VulkanImage::CreateImage(pRenderingContext, pTexture.m_Width, pTexture.m_Height, 4, pPixelData, true, &m_TextureImage);	// NOTE: We require 4 channels for now, even if there's no alpha channel

	m_TextureImage.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		imageBuffer.CopyBufferToImage(pRenderingContext, pTexture.m_Width, pTexture.m_Height, &m_TextureImage);
	m_TextureImage.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanBuffer::DestroyBuffer(pRenderingContext, &imageBuffer);

	pTexture.m_internalData = &m_TextureImage;

	return true;
}

bool VulkanShader::AcquireTextureResources(CVulkanRenderer* pRenderingContext, Texture& pTexture, void* pPixelData)
{
	VkDeviceSize imageSize = pTexture.m_Width * pTexture.m_Height * 4;

	VulkanBuffer imageBuffer;
	VulkanBuffer::CreateBuffer(pRenderingContext, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer);

	void* data;
	vkMapMemory(pRenderingContext->m_Device.m_LogicalDevice, imageBuffer.m_Memory, 0, imageSize, 0, &data);
		Memory::MemCopy(data, pPixelData, static_cast<size_t>(imageSize));
	vkUnmapMemory(pRenderingContext->m_Device.m_LogicalDevice, imageBuffer.m_Memory);

	// Create Vulkan Resources for image
	VulkanImage::CreateImage(pRenderingContext, pTexture.m_Width, pTexture.m_Height, 4, pPixelData, true, &m_TextureImage);	// NOTE: We require 4 channels for now, even if there's no alpha channel

	m_TextureImage.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		imageBuffer.CopyBufferToImage(pRenderingContext, pTexture.m_Width, pTexture.m_Height, &m_TextureImage);
	m_TextureImage.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanBuffer::DestroyBuffer(pRenderingContext, &imageBuffer);

	pTexture.m_internalData = &m_TextureImage;

	return true;
}

bool VulkanShader::CreateDescriptorResources(CVulkanRenderer* pRenderingContext)
{
	// Descriptor set layouts
	// TODO: Read in from shader config
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (!VK_CHECK(vkCreateDescriptorSetLayout(pRenderingContext->m_Device.m_LogicalDevice, &layoutInfo, pRenderingContext->m_Allocator, &m_DescriptorSetLayout)))
	{
		LOG_ERROR("Failed to create descriptor set layout!");
		return false;
	}

	// Descriptor Pool
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.poolSizeCount = 2;
	poolCreateInfo.pPoolSizes = poolSizes;
	poolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

	if (!VK_CHECK(vkCreateDescriptorPool(pRenderingContext->m_Device.m_LogicalDevice, &poolCreateInfo, pRenderingContext->m_Allocator, &m_DescriptorPool)))
	{
		LOG_ERROR("Failed to create Vulkan Descriptor Pool");
		return false;
	}

	// Descriptor Sets
	// TODO: Read in from shader config
	CVector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT);
	for (i32 i = 0; i < layouts.capacity(); i++)
	{
		layouts[i] = m_DescriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	descriptorSetAllocInfo.pSetLayouts = layouts.data();

	if (!VK_CHECK(vkAllocateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, &descriptorSetAllocInfo, m_DescriptorSets)))
	{
		LOG_ERROR("Failed to allocate descriptor sets!");
		return false;
	}

	for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffers[i].m_Handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_TextureImage.m_ImageView;
		imageInfo.sampler = m_TextureImage.m_ImageSampler;

		CVector<VkWriteDescriptorSet> descriptorWrites(2);
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		VkWriteDescriptorSet descriptorWriteArray[2] = { descriptorWrites[0], descriptorWrites[1] };

		vkUpdateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, 2, descriptorWriteArray, 0, nullptr);
	}

	return true;
}

void VulkanShader::UpdateDescriptorSets(CVulkanRenderer* pRenderingContext)
{
	for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffers[i].m_Handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_TextureImage.m_ImageView;
		imageInfo.sampler = m_TextureImage.m_ImageSampler;

		CVector<VkWriteDescriptorSet> descriptorWrites(2);
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		VkWriteDescriptorSet descriptorWriteArray[2] = { descriptorWrites[0], descriptorWrites[1] };

		vkUpdateDescriptorSets(pRenderingContext->m_Device.m_LogicalDevice, 2, descriptorWriteArray, 0, nullptr);
	}
}