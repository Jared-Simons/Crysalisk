//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: Interface for a Vulkan Renderpass
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"


bool VulkanRenderpass::CreateRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pOutRenderpass)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = pRenderingContext->m_Swapchain.m_SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (!VK_CHECK(vkCreateRenderPass(pRenderingContext->m_Device.m_LogicalDevice, &createInfo, pRenderingContext->m_Allocator, &pOutRenderpass->m_Handle)))
	{
		LOG_ERROR("Failed to create renderpass!");
		return false;
	}

	LOG_INFO("Main Renderpass Created");
	return true;
}

void VulkanRenderpass::DestroyRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pRenderpass)
{
	if (pRenderingContext && pRenderpass)
	{
		vkDestroyRenderPass(pRenderingContext->m_Device.m_LogicalDevice, pRenderpass->m_Handle, pRenderingContext->m_Allocator);
	}
}
