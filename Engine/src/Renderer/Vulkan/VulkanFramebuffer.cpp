//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2024
// 
// Purpose: interface and utilties for Vulkan Framebuffers
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanFramebuffer::CreateFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pOutFramebuffer, u32 framebufferIndex)
{
	if (!pOutFramebuffer)
	{
		LOG_ERROR("CreateFramebuffer() -> Requires a pointer to a valid VulkanFramebuffer object.");
		return false;
	}

	// TODO: Make configurable
	VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	createInfo.renderPass = pRenderingContext->m_MainRenderpass.m_Handle;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &pRenderingContext->m_Swapchain.m_SwapchainImageViews[framebufferIndex];
	createInfo.width = pRenderingContext->m_Swapchain.m_SwapchainExtent.width;
	createInfo.height = pRenderingContext->m_Swapchain.m_SwapchainExtent.height;
	createInfo.layers = 1;

	if (!VK_CHECK(vkCreateFramebuffer(pRenderingContext->m_Device.m_LogicalDevice, &createInfo, pRenderingContext->m_Allocator, &pOutFramebuffer->m_Handle)))
	{
		LOG_ERROR("Failed to create framebuffer");
		return false;
		framebufferIndex = 0;
	}

	return true;
}

void VulkanFramebuffer::DestroyFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pFramebuffer)
{
	if (pFramebuffer && pFramebuffer->m_Handle)
	{
		vkDestroyFramebuffer(pRenderingContext->m_Device.m_LogicalDevice, pFramebuffer->m_Handle, pRenderingContext->m_Allocator);
	}
}


