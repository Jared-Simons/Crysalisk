//=============================================================================
// Author: Jared Simons
// Date Created: 6/25/2024
// 
// Purpose: Interface/Utitlies for Vulkan Command Buffers
//=============================================================================

#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanCommandPool::CreateCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pOutPool)
{
	if (!pOutPool)
	{
		LOG_ERROR("CreateCommandPool() -> Function requires a pointer to a valid VulkanCommandPool");
		return false;
	}

	VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	// Command Buffers allocated from this pool will be set to reset state
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex = pRenderingContext->m_Device.m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex;

	if (!VK_CHECK(vkCreateCommandPool(pRenderingContext->m_Device.m_LogicalDevice, &poolCreateInfo, pRenderingContext->m_Allocator, &pOutPool->m_Handle)))
	{
		LOG_ERROR("Failed to create command pool");
		return false;
	}

	pOutPool->m_PoolFlags = poolCreateInfo.flags;
	LOG_INFO("Vulkan Command Pool Created");
	return true;
}

void VulkanCommandPool::DestroyCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pPool)
{
	if (pPool && pPool->m_Handle)
	{
		vkDestroyCommandPool(pRenderingContext->m_Device.m_LogicalDevice, pPool->m_Handle, pRenderingContext->m_Allocator);
	}
}

bool VulkanCommandBuffer::CreateCommandBuffer(CVulkanRenderer* pRenderingContext, bool isPrimary, VulkanCommandBuffer* pOutCommandBuffer)
{
	if (!pOutCommandBuffer)
	{
		LOG_ERROR("CreateCommandBuffer() -> Function requires a pointer to a valid VulkanCommandPool");
		return false;
	}

	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = pRenderingContext->m_CommandPool.m_Handle;
	allocateInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	if (!VK_CHECK(vkAllocateCommandBuffers(pRenderingContext->m_Device.m_LogicalDevice, &allocateInfo, &pOutCommandBuffer->m_Handle)))
	{
		LOG_ERROR("Failed to create command buffer");
		return false;
	}
	pOutCommandBuffer->m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INITIAL;

	return true;
}

void VulkanCommandBuffer::DestroyCommandBuffer(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer)
{
	if (pCommandBuffer && pCommandBuffer->m_Handle)
	{
		vkFreeCommandBuffers(pRenderingContext->m_Device.m_LogicalDevice, pRenderingContext->m_CommandPool.m_Handle, 1, &pCommandBuffer->m_Handle);
	}
}

