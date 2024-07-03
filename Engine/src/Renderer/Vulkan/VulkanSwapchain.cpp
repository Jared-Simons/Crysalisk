//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: Interface for a Vulkan Swapchain
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"
#include "Platform/Platform.h"
#include "EngineGlobals.h"

bool VulkanSwapchain::CreateSwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pOutSwapchain)
{
	pRenderingContext->m_Device.QuerySwapchainSupportDetails(pRenderingContext, &pOutSwapchain->m_SwapchainSupport);
	SwapchainSupportDetails* swapchainSupport = &pOutSwapchain->m_SwapchainSupport;

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };

	// Select image format
	for (unsigned int i = 0; i < swapchainSupport->formats.size(); i++)
	{
		if ((swapchainSupport->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (swapchainSupport->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
		{
			// These are our preferred format and color space.
			swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
			swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			break;
		}
	}

	if (swapchainCreateInfo.imageFormat == VK_FORMAT_UNDEFINED)
	{
		// If we can't find our preferred format, just select the first available.
		swapchainCreateInfo.imageFormat = swapchainSupport->formats[0].format;
		swapchainCreateInfo.imageColorSpace = swapchainSupport->formats[0].colorSpace;
	}

	// Select present mode
	for (unsigned int i = 0; i < swapchainSupport->presentModes.size(); i++)
	{
		if (swapchainSupport->presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	if (swapchainCreateInfo.presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	{
		// Fall back on this present mode if needed, as it is the only one guaranteed to exist.
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}

	swapchainCreateInfo.imageExtent = swapchainSupport->surfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers = swapchainSupport->surfaceCapabilities.maxImageArrayLayers;
	
	swapchainCreateInfo.minImageCount = swapchainSupport->surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ((swapchainCreateInfo.minImageCount < MAX_FRAMES_IN_FLIGHT) && (MAX_FRAMES_IN_FLIGHT < swapchainSupport->surfaceCapabilities.maxImageCount))
	{
		// Bump up the max image count in the case that MAX_FRAMES_IN_FLIGHT is higher than the minImageCount returned from the device query.
		swapchainCreateInfo.minImageCount = MAX_FRAMES_IN_FLIGHT;
	}

	swapchainCreateInfo.surface = pRenderingContext->m_Surface;
	swapchainCreateInfo.preTransform = swapchainSupport->surfaceCapabilities.currentTransform;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	if (!VK_CHECK(vkCreateSwapchainKHR(pRenderingContext->m_Device.m_LogicalDevice, &swapchainCreateInfo, pRenderingContext->m_Allocator, &pOutSwapchain->m_SwapchainHandle)))
	{
		LOG_ERROR("Failed to create swapchain");
		return false;
	}

	pOutSwapchain->m_SwapchainImageFormat = swapchainCreateInfo.imageFormat;
	pOutSwapchain->m_SwapchainExtent = swapchainCreateInfo.imageExtent;

	if (!pOutSwapchain->GetSwapchainImages(pRenderingContext))
	{
		LOG_ERROR("Failed to get swapchain images");
		return false;
	}

	if (!pOutSwapchain->CreateImageViews(pRenderingContext))
	{
		LOG_ERROR("Failed to create swapchain image views");
		return false;
	}

	LOG_INFO("Swapchain created");
	return true;
}

void VulkanSwapchain::DestroySwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain)
{
	if (pRenderingContext && pSwapchain && pSwapchain->m_SwapchainHandle)
	{
		for (int i = 0; i < pSwapchain->m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView(pRenderingContext->m_Device.m_LogicalDevice, pSwapchain->m_SwapchainImageViews[i], pRenderingContext->m_Allocator);
		}

		vkDestroySwapchainKHR(pRenderingContext->m_Device.m_LogicalDevice, pSwapchain->m_SwapchainHandle, pRenderingContext->m_Allocator);
	}
}

bool VulkanSwapchain::RecreateSwapchain(CVulkanRenderer* pRenderingContext, unsigned int newWidth, unsigned int newHeight)
{
	pRenderingContext->m_FramebufferWidth = newWidth;
	pRenderingContext->m_FramebufferHeight = newHeight;

	// Handle minimization
	if (newWidth == 0 || newHeight == 0)
		return true;
	
	vkDeviceWaitIdle(pRenderingContext->m_Device.m_LogicalDevice);

	DestroySwapchain(pRenderingContext, this);

	CreateSwapchain(pRenderingContext, this);
	
	// TOOD: Move framebuffers to the swapchain.
	for (int i = 0; i < pRenderingContext->m_Framebuffers.size(); i++)
		VulkanFramebuffer::DestroyFramebuffer(pRenderingContext, &pRenderingContext->m_Framebuffers[i]);

	for (int i = 0; i < pRenderingContext->m_Framebuffers.size(); i++)
	{
		if (!VulkanFramebuffer::CreateFramebuffer(pRenderingContext, &pRenderingContext->m_Framebuffers[i]))
			return false;
	}

	return true;
}

bool VulkanSwapchain::GetSwapchainImages(CVulkanRenderer* pRenderingContext)
{
	unsigned int imageCount{ 0 };
	vkGetSwapchainImagesKHR(pRenderingContext->m_Device.m_LogicalDevice, m_SwapchainHandle, &imageCount, nullptr);
	m_SwapchainImages.Resize(imageCount);
	vkGetSwapchainImagesKHR(pRenderingContext->m_Device.m_LogicalDevice, m_SwapchainHandle, &imageCount, m_SwapchainImages.data());

	LOG_INFO("Vulkan Swapchain Images Found");
	return true;
}

bool VulkanSwapchain::CreateImageViews(CVulkanRenderer* pRenderingContext)
{
	m_SwapchainImageViews.Resize(m_SwapchainImages.size());

	for (int i = 0; i < m_SwapchainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.image = m_SwapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_SwapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (!VK_CHECK(vkCreateImageView(pRenderingContext->m_Device.m_LogicalDevice, &createInfo, pRenderingContext->m_Allocator, &m_SwapchainImageViews[i])))
		{
			LOG_ERROR("Failed to create image views");
			return false;
		}
	}

	LOG_INFO("Vulkan Swapchain Image Views Created");
	return true;
}
