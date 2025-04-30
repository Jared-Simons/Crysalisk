//=============================================================================
// Author: Jared Simons
// Date Created: 2/17/2024
// 
// Purpose: Interface for Vulkan Buffers
//=============================================================================
#include "VulkanRenderer.h"

#include <Containers/CFreeList.h>
#include <Math/VertexTypes.h>

bool VulkanBuffer::CreateBuffer(CVulkanRenderer& pRenderingContext, size_t bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, bool bUseFreelist, VulkanBuffer& pOutBuffer)
{
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.flags = 0;
	bufferInfo.size = (VkDeviceSize)bufferSize;
	bufferInfo.usage = bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(pRenderingContext.m_Device.m_Device, &bufferInfo, pRenderingContext.m_Allocator, &pOutBuffer.m_Handle));

	// Get buffer memory
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(pRenderingContext.m_Device.m_Device, pOutBuffer.m_Handle, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = pRenderingContext.m_Device.FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

	VK_CHECK(vkAllocateMemory(pRenderingContext.m_Device.m_Device, &allocInfo, pRenderingContext.m_Allocator, &pOutBuffer.m_Memory));

	vkBindBufferMemory(pRenderingContext.m_Device.m_Device, pOutBuffer.m_Handle, pOutBuffer.m_Memory, 0);

	pOutBuffer.m_bUsesFreelist = bUseFreelist;
	if (bUseFreelist)
	{
		// Set up our freelist.
		pOutBuffer.m_Freelist.SetNumBytesToTrack(bufferSize);
		// TODO: Add the option to have this memory owned/allocated by the freelist rather than passing it in.
		size_t freelistMemoryRequirement = pOutBuffer.m_Freelist.GetInternalMemoryRequirement();
		pOutBuffer.m_pFreelistMemory = new uint8_t[freelistMemoryRequirement];
		pOutBuffer.m_Freelist.InitFreelist(pOutBuffer.m_pFreelistMemory);
	}

	pOutBuffer.m_BufferSize = bufferSize;
	pOutBuffer.m_BytesAllocated = 0;

	return true;
}

void VulkanBuffer::DestroyBuffer(CVulkanRenderer& pRenderingContext, VulkanBuffer& pBuffer)
{
	if (pBuffer.m_pFreelistMemory)
	{
		// Release our freelist memory.
		delete[] pBuffer.m_pFreelistMemory;
		pBuffer.m_pFreelistMemory = nullptr;
	}

	vkFreeMemory(pRenderingContext.m_Device.m_Device, pBuffer.m_Memory, pRenderingContext.m_Allocator);
	vkDestroyBuffer(pRenderingContext.m_Device.m_Device, pBuffer.m_Handle, pRenderingContext.m_Allocator);
}

size_t VulkanBuffer::UploadDataRange(CVulkanRenderer& pRenderingContext, size_t dataSize, void* pData)
{
	if (dataSize <= 0)
	{
		LOG_ERROR("VulkanBuffer::UploadVertexData() -> Requires nonzero size!");
		return kInvalidID_U64;
	}

	size_t dataOffset = m_BytesAllocated;

	// If we're tracking memory via a freelist, get the memory range from there.
	if (m_bUsesFreelist)
	{
		dataOffset = m_Freelist.Allocate(dataSize);
	}

	void* pBufferData;
	vkMapMemory(pRenderingContext.m_Device.m_Device, m_Memory, m_BytesAllocated, dataSize, 0, &pBufferData);
	memcpy(pBufferData, pData, dataSize);
	vkUnmapMemory(pRenderingContext.m_Device.m_Device, m_Memory);

	size_t outOffset = m_BytesAllocated;
	m_BytesAllocated += dataSize;

	return outOffset;
}

void VulkanBuffer::FreeDataRange(CVulkanRenderer& pRenderingContext, size_t dataOffset, size_t dataSize)
{
	// This only works for buffers that track their memory with a freelist.
	if (!m_bUsesFreelist)
	{
		return;
	}

	// Mark the range of memory as being 'free' in the freelist.
	// NOTE: We're not gonna free this range in the buffer for now to avoid mapping/unmapping the block.
	// It will be up to the renderer to detect this data as being invalid and not using it anymore.
	m_Freelist.Free(dataSize, dataOffset);
}

bool VulkanCommandBuffer::CreateCommandBuffer(CVulkanRenderer& pRenderingContext, bool bIsPrimary, VulkanCommandBuffer& pOutCommandBuffer)
{
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = pRenderingContext.m_GraphicsCommandPool;
	allocateInfo.level = (bIsPrimary) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VK_CHECK(vkAllocateCommandBuffers(pRenderingContext.m_Device.m_Device, &allocateInfo, &pOutCommandBuffer.m_Handle));

	return true;
}

void VulkanCommandBuffer::DestroyCommandBuffer(class CVulkanRenderer& pRenderingContext, VulkanCommandBuffer& pCommandBuffer)
{
	vkFreeCommandBuffers(pRenderingContext.m_Device.m_Device, pRenderingContext.m_GraphicsCommandPool, 1, &pCommandBuffer.m_Handle);
}

bool VulkanCommandBuffer::CreateAndBeginSingleUse(CVulkanRenderer& pRenderingContext, VkCommandPool pCommandPool)
{
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pCommandPool;
	allocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(pRenderingContext.m_Device.m_Device, &allocInfo, &m_Handle));

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(m_Handle, &beginInfo));
	return true;
}

void VulkanCommandBuffer::EndSingleUse(class CVulkanRenderer& pRenderingContext, VkCommandPool pCommandPool)
{
	VK_CHECK(vkEndCommandBuffer(m_Handle));

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_Handle;

	vkQueueSubmit(pRenderingContext.m_Device.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(pRenderingContext.m_Device.m_GraphicsQueue);

	vkFreeCommandBuffers(pRenderingContext.m_Device.m_Device, pCommandPool, 1, &m_Handle);
}