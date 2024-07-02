//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2024
// 
// Purpose: Interface and utilties for Vulkan Framebuffers
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanFramebuffer::CreateFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pOutFramebuffer)
{
	if (!pOutFramebuffer)
	{
		LOG_ERROR("CreateFramebuffer() -> Requires a pointer to a valid VulkanFramebuffer object.");
		return false;
	}
	// TODO: Temp value to make this more configurable.
	static int framebufferIndex{ 0 };

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
	}

	framebufferIndex++;
	if (framebufferIndex == 3)
	{
		// HACK: By having this set here, we require all 3 main framebuffers to be recreated at once AND we cannot create framebuffers for other purposes.
		// This should be refactored.
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


