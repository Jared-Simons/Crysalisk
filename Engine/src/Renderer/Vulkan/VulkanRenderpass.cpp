//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: interface for a Vulkan Renderpass
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"


bool VulkanRenderpass::CreateRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pOutRenderpass)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = pRenderingContext->m_Swapchain.GetImageFormat();
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

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	if (!VK_CHECK(vkCreateRenderPass(pRenderingContext->m_Device.GetLogicalDevice(), &createInfo, pRenderingContext->m_Allocator, &pOutRenderpass->m_Handle)))
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
		vkDestroyRenderPass(pRenderingContext->m_Device.GetLogicalDevice(), pRenderpass->m_Handle, pRenderingContext->m_Allocator);
	}
}

bool VulkanRenderpass::Begin(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer, u32 imageIndex)
{
	if (!m_Handle || !pCommandBuffer || !pCommandBuffer->GetHandle())
	{
		LOG_ERROR("VulkanRenderpass::Begin() -> Requires a valid renderpass handle and command buffer.");
		return false;
	}

	// TODO: Make this configurable
	VkRenderPassBeginInfo renderpassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderpassInfo.renderPass = m_Handle;
	renderpassInfo.framebuffer = pRenderingContext->m_Swapchain.GetFramebuffers().data()[imageIndex].m_Handle;
	renderpassInfo.renderArea.offset = { 0, 0 };
	renderpassInfo.renderArea.extent = pRenderingContext->m_Swapchain.GetExtent();

	VkClearValue clearColor = { 0.0f, 0.2f, 0.2f };
	renderpassInfo.clearValueCount = 1;
	renderpassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(pCommandBuffer->GetHandle(), &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
	return true;
}

void VulkanRenderpass::End(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer)
{
	if (m_Handle && pCommandBuffer && pCommandBuffer->GetHandle())
	{
		vkCmdEndRenderPass(pCommandBuffer->GetHandle());
		return;
	}
	else
	{
		LOG_ERROR("VulkanRenderpass::End() -> Requires a valid renderpass and command buffer.");
		return;
	}
}
