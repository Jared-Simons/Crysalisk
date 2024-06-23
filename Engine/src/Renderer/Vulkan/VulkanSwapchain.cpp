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

	LOG_INFO("Swapchain created");
	return true;
}

void VulkanSwapchain::DestroySwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain)
{
	if (pRenderingContext && pSwapchain && pSwapchain->m_SwapchainHandle)
	{
		vkDestroySwapchainKHR(pRenderingContext->m_Device.m_LogicalDevice, pSwapchain->m_SwapchainHandle, pRenderingContext->m_Allocator);
	}
}
