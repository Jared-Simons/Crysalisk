//=============================================================================
// Author: Jared Simons
// Date Created: 7/3/2024
// 
// Purpose: Methods and utilities for Vulkan Buffers
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanBuffer::CreateBuffer(CVulkanRenderer* pRenderingContext, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer* pOutBuffer)
{
	if (!pOutBuffer)
	{
		LOG_ERROR("VulkanBuffer::CreateBuffer() -> Requires a valid pointer to a Vulkan Buffer object.");
		return false;
	}

	VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CHECK(vkCreateBuffer(pRenderingContext->m_Device.GetLogicalDevice(), &bufferCreateInfo, pRenderingContext->m_Allocator, &pOutBuffer->m_Handle)))
	{
		LOG_ERROR("VulKanBuffer::CreateBuffer(): Failed to create buffer.");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(pRenderingContext->m_Device.GetLogicalDevice(), pOutBuffer->m_Handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = pRenderingContext->m_Device.FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (!VK_CHECK(vkAllocateMemory(pRenderingContext->m_Device.GetLogicalDevice(), &allocInfo, pRenderingContext->m_Allocator, &pOutBuffer->m_Memory)))
	{
		LOG_ERROR("VulKanBuffer::CreateBuffer(): Failed to allocate buffer memory.");
		return false;
	}

	vkBindBufferMemory(pRenderingContext->m_Device.GetLogicalDevice(), pOutBuffer->m_Handle, pOutBuffer->m_Memory, 0);

	return true;
}

void VulkanBuffer::DestroyBuffer(CVulkanRenderer* pRenderContext, VulkanBuffer* pBuffer)
{
	if (pBuffer && pBuffer->m_Handle && pBuffer->m_Memory)
	{
		vkFreeMemory(pRenderContext->m_Device.GetLogicalDevice(), pBuffer->m_Memory, pRenderContext->m_Allocator);
		vkDestroyBuffer(pRenderContext->m_Device.GetLogicalDevice(), pBuffer->m_Handle, pRenderContext->m_Allocator);
	}
	else
	{
		LOG_ERROR("VulKanBuffer::DestroyBuffer() -> Requires a valid pointer to a Vulkan Buffer object.");
		return;
	}
}

void VulkanBuffer::UploadDataToBuffer(CVulkanRenderer* pRenderingContext, u64 dataSize, void* data, u64 offset) const
{
	if (!data || dataSize <= 0)
	{
		LOG_ERROR("VulkanBuffer::UploadDataToBuffer() -> Requires valid data and data size.");
		return;
	}

	void* bufferData;
	vkMapMemory(pRenderingContext->m_Device.GetLogicalDevice(), m_Memory, offset, dataSize, 0, &bufferData);
		memcpy(bufferData, data, dataSize);
	vkUnmapMemory(pRenderingContext->m_Device.GetLogicalDevice(), m_Memory);
}

bool VulkanBuffer::CopyBufferToImage(CVulkanRenderer* pRenderingContext, u64 width, u64 height, VulkanImage* pImage) const
{
	VulkanCommandBuffer commandBuffer;
	commandBuffer.BeginSingleUse(pRenderingContext, &pRenderingContext->m_GraphicsCommandPool);

	VkBufferImageCopy copyRegion{};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { static_cast<u32>(width), static_cast<u32>(height), 1 };

	vkCmdCopyBufferToImage(commandBuffer.GetHandle(), m_Handle, pImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	commandBuffer.EndSingleUse(pRenderingContext, &pRenderingContext->m_GraphicsCommandPool);
	return true;
}