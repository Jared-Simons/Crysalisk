//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: interface for a Vulkan Renderpass
//=============================================================================
#include "VulkanRenderer.h"

bool VulkanRenderPass::CreateRenderpass(CVulkanRenderer& pRenderingContext, bool bShouldClear, VulkanRenderPass& pOutRenderpass)
{
	// TODO: Make this configurable with some config.
	Utl::CVector<VkAttachmentDescription> attachments;
	Utl::CVector<VkSubpassDescription> subpasses;

	// Color attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.flags = 0;
	colorAttachment.format = pRenderingContext.m_Swapchain.m_SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = bShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = bShouldClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments.PushBack(colorAttachment);

	// Depth attachment
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = pRenderingContext.m_Swapchain.m_DepthImageFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = bShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = bShouldClear ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE; // TODO: This is a bad heuristic
	depthAttachment.stencilLoadOp = bShouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.stencilStoreOp = bShouldClear ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE; // TODO: This is a bad heuristic
	depthAttachment.initialLayout = bShouldClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments.PushBack(depthAttachment);

	// Color attachment reference
	VkAttachmentReference colorAttachRef{};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment reference
	VkAttachmentReference depthAttachRef{};
	depthAttachRef.attachment = 1;
	depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Main subpass
	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef;
	subpass.pDepthStencilAttachment = &depthAttachRef;

	subpasses.PushBack(subpass);

	// Subpass dependency
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderpassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderpassInfo.attachmentCount = static_cast<uint32_t>(attachments.Count());
	renderpassInfo.pAttachments = attachments.Data();
	renderpassInfo.subpassCount = static_cast<uint32_t>(subpasses.Count());
	renderpassInfo.pSubpasses = subpasses.Data();
	renderpassInfo.dependencyCount = 1;
	renderpassInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(pRenderingContext.m_Device.m_Device, &renderpassInfo, pRenderingContext.m_Allocator, &pOutRenderpass.m_Handle));

	return true;
}

void VulkanRenderPass::DestroyRenderpass(class CVulkanRenderer& pRenderingContext, VulkanRenderPass& pRenderpass)
{
	vkDestroyRenderPass(pRenderingContext.m_Device.m_Device, pRenderpass.m_Handle, pRenderingContext.m_Allocator);
}
