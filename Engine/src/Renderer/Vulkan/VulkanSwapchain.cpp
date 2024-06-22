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

bool VulkanSwapchain::CreateSwapchain(CVulkanRenderer* renderingContext)
{
	// We requery the device's swapchain support details here - just in case the framebuffer was recreated - or any other details changed.
	renderingContext->GetDevice().QueryDeviceSwapchainSupport(renderingContext, renderingContext->GetDevice().GetPhysicalDevice(), renderingContext->GetSwapchain());
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

	// NOTE: The 3 here is for triple-buffering - this will need to be swapped out for runtime swapping of double and triple buffering.
	for (int i = 0; i < 3; i++)
	{
		vkDestroyFramebuffer(renderingContext->GetDevice().GetLogicalDevice(), m_Framebuffers[i], nullptr);
	}

	for (int i = 0; i < m_SwapchainImageViews.GetCount(); i++)
	{
		vkDestroyImageView(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(renderingContext->GetDevice().GetLogicalDevice(), m_SwapchainHandle, nullptr);
}

bool VulkanSwapchain::RecreateSwapchain(CVulkanRenderer* renderingContext)
{
	unsigned int width, height;
	Platform_GetWindowSize(&width, &height);

	if (width == 0 || height == 0)
	{
		return false;
	}

	vkDeviceWaitIdle(renderingContext->GetDevice().GetLogicalDevice());

	CleanupSwapchain(renderingContext);

	CreateSwapchain(renderingContext);
	CreateImageViews(renderingContext);
	CreateFramebuffers(renderingContext);

	renderingContext->SetFramebufferResized(false);

	return true;
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
	unsigned int width, height;
	Platform_GetWindowSize(&width, &height);

	VkExtent2D min = capabilities.minImageExtent;
	VkExtent2D max = capabilities.maxImageExtent;
	VkExtent2D actualExtent = capabilities.currentExtent;
	IntClamp((int*)actualExtent.width, (int*)min.width, (int*)max.width);
	IntClamp((int*)actualExtent.height, (int*)min.height, (int*)max.height);
	return actualExtent;
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

bool VulkanSwapchain::CreateFramebuffers(CVulkanRenderer* renderingContext)
{
	// TODO: This will link to our single renderpass for output
	// This will need to be more configurable asap.
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.flags = 0;
		createInfo.layers = 1;
		createInfo.renderPass = renderingContext->GetRenderpass().GetHandle();
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &m_SwapchainImageViews[i];
		createInfo.width = m_SwapchainExtent.width;
		createInfo.height = m_SwapchainExtent.height;

		// TODO: Custom allocator
		VkResult result = vkCreateFramebuffer(renderingContext->GetDevice().GetLogicalDevice(), &createInfo, nullptr, &m_Framebuffers[i]);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create swapchain framebuffers!");
			return false;
		}
	}

	LOG_INFO("Swapchain framebuffers created");
	return true;
}

void VulkanSwapchain::DestroyFramebuffers(CVulkanRenderer* renderingContext)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// TODO: Custom allocator
		vkDestroyFramebuffer(renderingContext->GetDevice().GetLogicalDevice(), m_Framebuffers[i], nullptr);
	}
}

bool VulkanSwapchain::FramebufferPresent(CVulkanRenderer* renderingContext, unsigned int currImageIndex)
{
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderingContext->GetRenderFinishedSemaphore(currImageIndex);
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_SwapchainHandle;
	presentInfo.pImageIndices = &currImageIndex;

	VkResult presentResult = vkQueuePresentKHR(renderingContext->GetDevice().GetPresentQueue(), &presentInfo);
	if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR)
	{
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			renderingContext->GetSwapchain().RecreateSwapchain(renderingContext);
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

