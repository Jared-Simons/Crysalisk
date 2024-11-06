//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2024
// 
// Purpose: interface and utilties for Vulkan Fences
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanFence::CreateFence(CVulkanRenderer* pRenderingContext, bool bCreateSignaled, VulkanFence* pOutFence)
{
	if (!pOutFence)
	{
		LOG_ERROR("CreateFence() -> Requires a pointer to a valid VulkanFence object.");
		return false;
	}

	VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	createInfo.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if (!VK_CHECK(vkCreateFence(pRenderingContext->m_Device.GetLogicalDevice(), &createInfo, pRenderingContext->m_Allocator, &pOutFence->m_Handle)))
	{
		LOG_ERROR("Failed to create fence");
		return false;
	}

	return true;
}

void VulkanFence::DestroyFence(CVulkanRenderer* pRenderingContext, VulkanFence* pFence)
{
	if (pFence && pFence->m_Handle)
	{
		vkDestroyFence(pRenderingContext->m_Device.GetLogicalDevice(), pFence->m_Handle, pRenderingContext->m_Allocator);
	}
}
