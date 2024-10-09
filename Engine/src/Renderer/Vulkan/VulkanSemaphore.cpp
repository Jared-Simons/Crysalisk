//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2024
// 
// Purpose: interface and utilties for Vulkan Semaphore
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanSemaphore::CreateVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pOutSemaphore)
{
	if (!pOutSemaphore)
	{
		LOG_ERROR("CreateVulkanSemaphore() -> Requires a pointer to a valid VulkanSemaphore object.");
		return false;
	}

	VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	if (!VK_CHECK(vkCreateSemaphore(pRenderingContext->m_Device.m_LogicalDevice, &createInfo, pRenderingContext->m_Allocator, &pOutSemaphore->m_Handle)))
	{
		LOG_ERROR("Failed to create semaphore.");
		return false;
	}

	return true;
}

void VulkanSemaphore::DestroyVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pSemaphore)
{
	if (pSemaphore && pSemaphore->m_Handle)
	{
		vkDestroySemaphore(pRenderingContext->m_Device.m_LogicalDevice, pSemaphore->m_Handle, pRenderingContext->m_Allocator);
	}
}
