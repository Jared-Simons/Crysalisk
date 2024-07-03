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
	allocateInfo.commandPool = pRenderingContext->m_GraphicsCommandPool.m_Handle;
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
		vkFreeCommandBuffers(pRenderingContext->m_Device.m_LogicalDevice, pRenderingContext->m_GraphicsCommandPool.m_Handle, 1, &pCommandBuffer->m_Handle);
	}
}

bool VulkanCommandBuffer::Begin()
{
	if (!m_Handle)
	{
		LOG_ERROR("VulkanCommandBuffer::Begin() -> Requires a pointer to a valid command buffer object.");
		return false;
	}

	if (m_State != eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INITIAL)
	{
		LOG_ERROR("VulkanCommandBuffer::Begin() -> Command buffer was not in initial state, thus could not begin recording commands.");
		return false;
	}

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	if (!VK_CHECK(vkBeginCommandBuffer(m_Handle, &beginInfo)))
	{
		LOG_ERROR("VulkanCommandBuffer::Begin() -> Command buffer failed to begin recording.");
		return false;
	}

	m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_RECORDING;
	return true;
}

void VulkanCommandBuffer::End()
{
	if (m_Handle && m_State == eVulkanCommandBufferState::COMMAND_BUFFER_STATE_RECORDING)
	{
		if (!VK_CHECK(vkEndCommandBuffer(m_Handle)))
		{
			LOG_ERROR("VulkanCommandBuffer::End() -> Command buffer failed to stop recording.");
			return;
		}

		m_State = eVulkanCommandBufferState::COMMAND_BUFFER_EXECUTABLE;
	}
	else
	{
		LOG_ERROR("VulkanCommandBuffer::End() -> Command buffer either does not exist or was not in a recording state.");
		return;
	}
}

void VulkanCommandBuffer::Reset()
{
	if (m_Handle)
	{
		vkResetCommandBuffer(m_Handle, 0);
		m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INITIAL;
	}
	else
	{
		LOG_ERROR("VulkanCommandBuffer::Reset() -> Requires a handle to a valid command buffer object.");
	}
}

bool VulkanCommandBuffer::Submit(CVulkanRenderer* pRenderingContext)
{
	if (!m_Handle)
	{
		LOG_ERROR("VulkanCommandBuffer::Submit() -> Requires a pointer to a valid command buffer object.");
		m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INVALID;
		return false;
	}

	if (m_State != eVulkanCommandBufferState::COMMAND_BUFFER_EXECUTABLE)
	{
		LOG_ERROR("VulkanCommandBuffer::Submit() -> Command buffer is in an invalid state and cannot be submitted.");
		return false;
	}

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &pRenderingContext->m_ImageAvailableSemaphores[pRenderingContext->m_CurrentFrame].m_Handle;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_Handle;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &pRenderingContext->m_RenderFinishedSemaphores[pRenderingContext->m_CurrentFrame].m_Handle;

	if (!VK_CHECK(vkQueueSubmit(pRenderingContext->m_Device.m_GraphicsQueue, 1, &submitInfo, pRenderingContext->m_FrameInFlightFences[pRenderingContext->m_CurrentFrame].m_Handle)))
	{
		LOG_ERROR("Failed to submit command buffer");
		return false;
	}
	m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_PENDING;

	return true;
}