//=============================================================================
// Author: Jared Simons
// Date Created: 2/17/2024
// 
// Purpose: Interface for Vulkan Buffers
//=============================================================================
#include "VulkanRenderer.h"

#include <Containers/CFreeList.h>
#include <Math/VertexTypes.h>

//-------------------------------------------------------------------------
// Purpose: Creates and acquires the resources for a Vulkan buffer.
// Input:	pRenderingContext - The owning rendering context.
//			bufferSize - The size in bytes that the buffer should have. Must be > 0.
//			bufferUsgae - Flags determining the buffer's usage.
//			memoryProperties - Flags determining the buffer's associated memory properties.
//			bUseFreelist - Determines if the buffer's memory should be tracked using a freelist (otherwise will use a linear allocation method).
//			pOutBuffer - The structure that will hold the created buffer.
// Output:	bool - Returns true on successful buffer creation, otherwise false.
//-------------------------------------------------------------------------
bool VulkanBuffer::CreateBuffer(CVulkanRenderer& pRenderingContext, u64 bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, bool bUseFreelist, VulkanBuffer& pOutBuffer)
{
	if (bufferSize <= 0)
	{
		LOG_ERROR("Cannot create a Vulkan buffer with a size < 1. Size provided: %d", bufferSize);
		return false;
	}

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
	pOutBuffer.m_MemoryProperties = memoryProperties;
	pOutBuffer.m_BufferUsage = bufferUsage;

	pOutBuffer.m_bUsesFreelist = bUseFreelist;
	if (bUseFreelist)
	{
		// Set up our freelist.
		pOutBuffer.m_Freelist.SetNumBytesToTrack(bufferSize);
		u64 freelistMemoryRequirement = pOutBuffer.m_Freelist.GetInternalMemoryRequirement();
		pOutBuffer.m_pFreelistMemory = new u8[freelistMemoryRequirement];
		pOutBuffer.m_Freelist.InitFreelist(pOutBuffer.m_pFreelistMemory);
	}

	pOutBuffer.m_BufferSize = bufferSize;
	pOutBuffer.m_BytesAllocated = 0;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases Vulkan buffer resources.
// Input:	pRenderingContext - The owning rendering context.
//			pBuffer - The buffer to be released.
// Output:	none.
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// Purpose: Resizes the Vulkan buffer.
//			Will retain all data between the allocations.
//			Does nothing if iNewBufferSize is less than what the buffer currently holds.
// Input:	pRenderingContext - The owning rendering context.
//			pBuffer - The buffer to be resized.
//			iNewBufferSize - The new size of the buffer in bytes.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanBuffer::Resize(CVulkanRenderer& pRenderingContext, VulkanBuffer* pBuffer, u64 iNewBufferSize)
{
	if (pBuffer->m_BufferSize >= iNewBufferSize || iNewBufferSize < 1)
	{
		// No buffer shrinking.
		return;
	}

	// Create a new buffer and allocate it's memory.	
	VkBuffer		newBuffer{};
	VkDeviceMemory	newBufferMemory{};

	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.flags = 0;
	bufferInfo.size = (VkDeviceSize)iNewBufferSize;
	bufferInfo.usage = pBuffer->m_BufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // Need to be marked as a destination buffer for the copy operation.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(pRenderingContext.m_Device.m_Device, &bufferInfo, pRenderingContext.m_Allocator, &newBuffer));

	// Get buffer memory
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(pRenderingContext.m_Device.m_Device, newBuffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = pRenderingContext.m_Device.FindMemoryType(memoryRequirements.memoryTypeBits, pBuffer->m_MemoryProperties);

	VK_CHECK(vkAllocateMemory(pRenderingContext.m_Device.m_Device, &allocInfo, pRenderingContext.m_Allocator, &newBufferMemory));

	vkBindBufferMemory(pRenderingContext.m_Device.m_Device, newBuffer, newBufferMemory, 0);

	// Copy over the region from the old range to the new.
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = pBuffer->m_BufferSize;

	// Start the command buffer to handle the copy.
	VulkanCommandBuffer		commandBuffer{};
	commandBuffer.CreateAndBeginSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);
	vkCmdCopyBuffer(commandBuffer.m_Handle, pBuffer->m_Handle, newBuffer, 1, &copyRegion);
	commandBuffer.EndSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);

	// Make sure resources are no longer in use prior to destruction.
	vkDeviceWaitIdle(pRenderingContext.m_Device.m_Device);

	// Destroy the old buffer and reassign the handles.
	vkFreeMemory(pRenderingContext.m_Device.m_Device, pBuffer->m_Memory, pRenderingContext.m_Allocator);
	vkDestroyBuffer(pRenderingContext.m_Device.m_Device, pBuffer->m_Handle, pRenderingContext.m_Allocator);
	pBuffer->m_Handle = newBuffer;
	pBuffer->m_Memory = newBufferMemory;
}

//-------------------------------------------------------------------------
// Purpose: Uploads data to the buffer.
// Input:	pRenderingContext - The owning rendering context.
//			dataSize - The size of data in bytes to be uploaded.
//			pData - An opaque pointer to the data to be uploaded.
// Output:	The offset in the buffer that this data was placed.
//-------------------------------------------------------------------------
u64 VulkanBuffer::UploadDataRange(CVulkanRenderer& pRenderingContext, u64 dataSize, void* pData)
{
	if (dataSize <= 0)
	{
		LOG_ERROR("VulkanBuffer::UploadVertexData() -> Requires nonzero size!");
		return kInvalidID_U64;
	}

	u64 dataOffset = m_BytesAllocated;

	// If we're tracking memory via a freelist, get the memory range from there.
	if (m_bUsesFreelist)
	{
		dataOffset = m_Freelist.Allocate(dataSize);
	}

	void* pBufferData;
	vkMapMemory(pRenderingContext.m_Device.m_Device, m_Memory, m_BytesAllocated, dataSize, 0, &pBufferData);
	memcpy(pBufferData, pData, dataSize);
	vkUnmapMemory(pRenderingContext.m_Device.m_Device, m_Memory);

	u64 outOffset = m_BytesAllocated;
	m_BytesAllocated += dataSize;

	return outOffset;
}

//-------------------------------------------------------------------------
// Purpose: Frees a data range in the buffer.
// Input:	pRenderingContext - The owning rendering context.
//			dataOffset - The offset in the buffer this data lives.
//			dataSize - The size of data in bytes to be freed.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanBuffer::FreeDataRange(CVulkanRenderer& pRenderingContext, u64 dataOffset, u64 dataSize)
{
	// Deduct the size from the count.
	m_BytesAllocated -= dataSize;

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

//-------------------------------------------------------------------------
// Purpose: Factory method that creates command buffers.
// Input:	pRenderingContext - The owning rendering context.
//			bIsPrimary - Indicates if this command buffer should be primary level or secondary level.
//			pOutCommandBuffer - The structure that will hold the created commandbuffer.
// Output:	bool - Returns true on successful creation, otherwise false.
//-------------------------------------------------------------------------
bool VulkanCommandBuffer::CreateCommandBuffer(CVulkanRenderer& pRenderingContext, bool bIsPrimary, VulkanCommandBuffer& pOutCommandBuffer)
{
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = pRenderingContext.m_GraphicsCommandPool;
	allocateInfo.level = (bIsPrimary) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VK_CHECK(vkAllocateCommandBuffers(pRenderingContext.m_Device.m_Device, &allocateInfo, &pOutCommandBuffer.m_Handle));

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases command buffers.
// Input:	pRenderingContext - The owning rendering context.
//			pCommandBuffer - The command buffer to be released.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanCommandBuffer::DestroyCommandBuffer(CVulkanRenderer& pRenderingContext, VulkanCommandBuffer& pCommandBuffer)
{
	vkFreeCommandBuffers(pRenderingContext.m_Device.m_Device, pRenderingContext.m_GraphicsCommandPool, 1, &pCommandBuffer.m_Handle);
}

//-------------------------------------------------------------------------
// Purpose: Creates and begins a single-use command buffer.
// Input:	pRenderingContext - The owning rendering context.
//			pCommandPool - The command pool to allocate this buffer from.
// Output:	bool - Returns true on successful creation, otherwise false.
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// Purpose: Ends and submits a single-use command buffer.
// Input:	pRenderingContext - The owning rendering context.
//			pCommandPool - The command pool that allocated this buffer.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanCommandBuffer::EndSingleUse(CVulkanRenderer& pRenderingContext, VkCommandPool pCommandPool)
{
	VK_CHECK(vkEndCommandBuffer(m_Handle));

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_Handle;

	vkQueueSubmit(pRenderingContext.m_Device.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(pRenderingContext.m_Device.m_GraphicsQueue);

	vkFreeCommandBuffers(pRenderingContext.m_Device.m_Device, pCommandPool, 1, &m_Handle);
}