//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: interface for a Vulkan Swapchain
//=============================================================================
#include "VulkanRenderer.h"

//-------------------------------------------------------------------------
// Contains various details about swapchain support.
//-------------------------------------------------------------------------
struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR			SurfaceCapabilities;
	Utl::CVector<VkSurfaceFormatKHR>	Formats;
	Utl::CVector<VkPresentModeKHR>		PresentModes;
};

//-------------------------------------------------------------------------
// Purpose: Creates and acquires the resources for a Vulkan swapchain.
// Input:	pRenderingContext - The owning rendering context.
//			pOutSwapchain - The structure that will hold the created swapchain.
// Output:	bool - Returns true on successful swapchain creation, otherwise false.
//-------------------------------------------------------------------------
bool VulkanSwapchain::CreateSwapchain(CVulkanRenderer& pRenderingContext, VulkanSwapchain& pOutSwapchain)
{
	// Query our device for swapchain support.
	SwapchainSupportDetails swapchainSupport{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pRenderingContext.m_Device.m_PhysicalDevice, pRenderingContext.m_Surface, &swapchainSupport.SurfaceCapabilities);

	unsigned int numFormats{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderingContext.m_Device.m_PhysicalDevice, pRenderingContext.m_Surface, &numFormats, nullptr);
	swapchainSupport.Formats.Resize(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderingContext.m_Device.m_PhysicalDevice, pRenderingContext.m_Surface, &numFormats, swapchainSupport.Formats.Data());

	unsigned int numPresentModes{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderingContext.m_Device.m_PhysicalDevice, pRenderingContext.m_Surface, &numPresentModes, nullptr);
	swapchainSupport.PresentModes.Resize(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderingContext.m_Device.m_PhysicalDevice, pRenderingContext.m_Surface, &numPresentModes, swapchainSupport.PresentModes.Data());

	unsigned int imageCount = swapchainSupport.SurfaceCapabilities.minImageCount;
	if (imageCount < pRenderingContext.m_MaxImagesInFlight && pRenderingContext.m_MaxImagesInFlight < swapchainSupport.SurfaceCapabilities.maxImageCount)
	{
		// Bump up the min image count in the case that MaxImagesInFlight is higher than the minImageCount returned from the device query.
		imageCount = pRenderingContext.m_MaxImagesInFlight;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };

	// Select image format and color space
	for (int i = 0; i < swapchainSupport.Formats.Count(); i++)
	{
		if ((swapchainSupport.Formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (swapchainSupport.Formats[i].colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
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
		swapchainCreateInfo.imageFormat = swapchainSupport.Formats[0].format;
		swapchainCreateInfo.imageColorSpace = swapchainSupport.Formats[0].colorSpace;
	}

	// Select present mode
	for (int i = 0; i < swapchainSupport.PresentModes.Count(); i++)
	{
		if (swapchainSupport.PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			// TODO: Add the option for FIFO for low-power environments?
			swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	if (swapchainCreateInfo.presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	{
		// Fall back on this present mode if needed, as it is the only one guaranteed to exist.
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}

	swapchainCreateInfo.imageExtent = swapchainSupport.SurfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers = swapchainSupport.SurfaceCapabilities.maxImageArrayLayers;
	swapchainCreateInfo.surface = pRenderingContext.m_Surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.preTransform = swapchainSupport.SurfaceCapabilities.currentTransform;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VK_CHECK(vkCreateSwapchainKHR(pRenderingContext.m_Device.m_Device, &swapchainCreateInfo, pRenderingContext.m_Allocator, &pOutSwapchain.m_Handle));

	// Cache off relevant formats.
	pOutSwapchain.m_SwapchainImageFormat = swapchainCreateInfo.imageFormat;
	pOutSwapchain.m_SwapchainExtent = swapchainCreateInfo.imageExtent;

	// Obtain the swapchain images.
	unsigned int swapImageCount{ 0 };
	vkGetSwapchainImagesKHR(pRenderingContext.m_Device.m_Device, pOutSwapchain.m_Handle, &swapImageCount, nullptr);
	pOutSwapchain.m_SwapchainImages.Resize(imageCount);
	vkGetSwapchainImagesKHR(pRenderingContext.m_Device.m_Device, pOutSwapchain.m_Handle, &swapImageCount, pOutSwapchain.m_SwapchainImages.Data());

	// Create the swapchain image views.
	pOutSwapchain.m_SwapchainImageViews.Resize(imageCount);
	for (unsigned int imageIdx = 0; imageIdx < imageCount; imageIdx++)
	{
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = pOutSwapchain.m_SwapchainImages[imageIdx];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = pOutSwapchain.m_SwapchainImageFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(pRenderingContext.m_Device.m_Device, &viewInfo, pRenderingContext.m_Allocator, &pOutSwapchain.m_SwapchainImageViews[imageIdx]));
	}

	// Query our device for possible depth formats.
	// TODO: We need to do error checking/handling here...
	pOutSwapchain.m_DepthImageFormat = pRenderingContext.m_Device.FindSupportedDepthFormat();

	// Create our depth image.
	if (!VulkanImage::CreateImage(pRenderingContext, "__Swapchain_Depth_Image", pOutSwapchain.m_SwapchainExtent.width, pOutSwapchain.m_SwapchainExtent.height, pOutSwapchain.m_DepthImageFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, pOutSwapchain.m_DepthImage))
	{
		LOG_ERROR("Failed to create swapchain depth buffer!");
		return false;
	}
	pOutSwapchain.m_DepthImage.TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases Vulkan swapchain resources.
// Input:	pRenderingContext - The owning rendering context.
//			pSwapchain - The swapchain to be released.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanSwapchain::DestroySwapchain(CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain)
{
	for (int imageIdx = 0; imageIdx < pSwapchain.m_Framebuffers.Capacity(); imageIdx++)
	{
		vkDestroyFramebuffer(pRenderingContext.m_Device.m_Device, pSwapchain.m_Framebuffers[imageIdx], pRenderingContext.m_Allocator);
	}

	VulkanImage::DestroyImage(pRenderingContext, pSwapchain.m_DepthImage);

	for (int imageIdx = 0; imageIdx < pSwapchain.m_SwapchainImageViews.Capacity(); imageIdx++)
	{
		vkDestroyImageView(pRenderingContext.m_Device.m_Device, pSwapchain.m_SwapchainImageViews[imageIdx], pRenderingContext.m_Allocator);
	}

	vkDestroySwapchainKHR(pRenderingContext.m_Device.m_Device, pSwapchain.m_Handle, pRenderingContext.m_Allocator);
}

//-------------------------------------------------------------------------
// Purpose: Creates and acquires the resources for swapchain-owned framebuffers.
// Input:	pRenderingContext - The owning rendering context.
//			pSwapchain - The structure that will hold the created framebuffers.
// Output:	bool - Returns true on successful framebuffer creation, otherwise false.
//-------------------------------------------------------------------------
bool VulkanSwapchain::CreateFramebuffers(CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain, VulkanRenderPass* pMainRenderpass)
{
	// Create our framebuffers
	pSwapchain.m_Framebuffers.Resize(pRenderingContext.m_MaxImagesInFlight);
	// TODO: Make this configurable.
	for (unsigned int imageIdx = 0; imageIdx < pRenderingContext.m_MaxImagesInFlight; imageIdx++)
	{
		VkImageView attachmentViews[2] = { pRenderingContext.m_Swapchain.m_SwapchainImageViews[imageIdx], pRenderingContext.m_Swapchain.m_DepthImage.m_View };

		VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = pMainRenderpass->m_Handle;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachmentViews;
		framebufferInfo.width = pRenderingContext.m_Swapchain.m_SwapchainExtent.width;
		framebufferInfo.height = pRenderingContext.m_Swapchain.m_SwapchainExtent.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(pRenderingContext.m_Device.m_Device, &framebufferInfo, pRenderingContext.m_Allocator, &pSwapchain.m_Framebuffers[imageIdx]));

		// Cache this off in case we need it later.
		pSwapchain.m_pFramebufferPass = pMainRenderpass;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Recreates the swapchain, when necessary (i.e. window-resize).
// Input:	pRenderingContext - The owning rendering context.
//			pSwapchain - The swapchain to be recreated.
// Output:	none.
//-------------------------------------------------------------------------
void VulkanSwapchain::RecreateSwapchain(CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain)
{
	LOG_DEBUG("Recreating swapchain");

	vkDeviceWaitIdle(pRenderingContext.m_Device.m_Device);

	DestroySwapchain(pRenderingContext, pSwapchain);
	CreateSwapchain(pRenderingContext, pSwapchain);
	CreateFramebuffers(pRenderingContext, pSwapchain, pSwapchain.m_pFramebufferPass);

	pRenderingContext.m_bResizingFramebuffer = false;
}