//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: Interface for a Vulkan Swapchain
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"

bool VulkanSwapchain::CreateSwapchain(CVulkanRenderer* renderingContext)
{
	// Grab the swapchain support details passed in on device creation.
	SwapchainSupportDetails_t& swapchainSupport = m_SwapchainSupportDetails;

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapchainSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = ChooseSwapchainExtent(renderingContext, swapchainSupport.capabilities);

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
	{
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = renderingContext->GetVulkanSurface();
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	DeviceQueueFamilyProperties_t& indices = renderingContext->GetDevice().GetPhysicalDeviceQueueIndices();
	uint32_t queueFamilyIndices[] = { indices.graphicsIndex, indices.presentIndex };

	if (indices.graphicsIndex != indices.presentIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// TODO: Custom Allocator.
	if (vkCreateSwapchainKHR(renderingContext->GetDevice().GetLogicalDevice(), &swapchainCreateInfo, nullptr, &m_SwapchainHandle) != VK_SUCCESS)
	{
		AssertMsg("Failed to create swapchain!");
	}

	GetSwapchainImages(renderingContext);

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;

	LOG_INFO("Vulkan Swapchain Created");
	return true;
}

void VulkanSwapchain::CleanupSwapchain(CVulkanRenderer* renderingContext)
{
	// TODO: Custom Allocator
	vkDestroySwapchainKHR(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainHandle, nullptr);
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapchainSurfaceFormat(CVector<VkSurfaceFormatKHR>& availableFormats)
{
	for (int i = 0; i < availableFormats.GetCount(); i++)
	{
		if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormats[i];
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapchainPresentMode(CVector<VkPresentModeKHR>& availablePresentModes)
{
	for (int i = 0; i < availablePresentModes.GetCount(); i++)
	{
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentModes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapchainExtent(CVulkanRenderer* renderingContext, const VkSurfaceCapabilitiesKHR& capabilities)
{
	// TODO: need to actually implement this with some utilities (clamp() and limits())
	return capabilities.currentExtent;
}

void VulkanSwapchain::GetSwapchainImages(CVulkanRenderer* renderingContext)
{
	uint32_t imageCount{ 0 };
	vkGetSwapchainImagesKHR(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainHandle, &imageCount, nullptr);
	m_SwapchainImages.Resize(imageCount);
	vkGetSwapchainImagesKHR(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainHandle, &imageCount, m_SwapchainImages.Data());

	LOG_INFO("Vulkan Swapchain Images Found");
}

bool VulkanSwapchain::CreateImageViews(CVulkanRenderer* renderingContext)
{
	m_SwapchainImageViews.Resize(m_SwapchainImages.GetCount());

	for (int i = 0; i < m_SwapchainImages.GetCount(); i++)
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

		// TODO: Custom Allocator
		if (vkCreateImageView(renderingContext->GetDevice().GetLogicalDevice(), &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
		{
			AssertMsg("VulkanRenderer - Failed to create image views!");
			return false;
		}
	}

	LOG_INFO("Vulkan Swapchain Image Views Created");
	return true;
}

void VulkanSwapchain::DestroyImageViews(CVulkanRenderer* renderingContext)
{
	for (int i = 0; i < m_SwapchainImageViews.GetCount(); i++)
	{
		vkDestroyImageView(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainImageViews[i], nullptr);
	}
}


