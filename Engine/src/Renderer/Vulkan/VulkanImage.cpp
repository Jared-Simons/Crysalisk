//=============================================================================
// Author: Jared Simons
// Date Created: 7/8/2024
// 
// Purpose: Methods and utilities for Vulkan Images
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"

bool VulkanImage::CreateImage(CVulkanRenderer* pRenderingContext, int width, int height, int numChannels, void* pPixelData, bool bCreateView, VulkanImage* pOutImage)
{
	VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // TODO: make configurable
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO: Check for transparency
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // TODO: make configurable
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	pOutImage->m_ImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	if (!VK_CHECK(vkCreateImage(pRenderingContext->m_Device.m_LogicalDevice, &imageCreateInfo, pRenderingContext->m_Allocator, &pOutImage->m_Handle)))
	{
		LOG_ERROR("Failed to create Vulkan Image");
		return false;
	}

	// Allocate the memory for the image
	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(pRenderingContext->m_Device.m_LogicalDevice, pOutImage->m_Handle, &imageMemoryRequirements);

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = imageMemoryRequirements.size;
	memAllocInfo.memoryTypeIndex = pRenderingContext->m_Device.FindMemoryType(imageMemoryRequirements.memoryTypeBits, properties); // TODO: Make configurable

	if (!VK_CHECK(vkAllocateMemory(pRenderingContext->m_Device.m_LogicalDevice, &memAllocInfo, pRenderingContext->m_Allocator, &pOutImage->m_ImageMemory)))
	{
		LOG_ERROR("Failed to allocate Vulkan Image memory");
		return false;
	}

	// TODO: Bind on create parameter?
	vkBindImageMemory(pRenderingContext->m_Device.m_LogicalDevice, pOutImage->m_Handle, pOutImage->m_ImageMemory, 0);

	// Create Image View
	if (bCreateView)
	{
		VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewCreateInfo.image = pOutImage->m_Handle;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // TODO: Make configurable
		viewCreateInfo.format = pOutImage->m_ImageFormat;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		if (!VK_CHECK(vkCreateImageView(pRenderingContext->m_Device.m_LogicalDevice, &viewCreateInfo, pRenderingContext->m_Allocator, &pOutImage->m_ImageView)))
		{
			LOG_ERROR("Failed to create Vulkan Image View");
			return false;
		}
	}

	// Create sampler
	{
		// TODO: Configurable parameters
		VkSamplerCreateInfo samplerCreateInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = pRenderingContext->m_Device.m_PhysicalDeviceProperties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (!VK_CHECK(vkCreateSampler(pRenderingContext->m_Device.m_LogicalDevice, &samplerCreateInfo, pRenderingContext->m_Allocator, &pOutImage->m_ImageSampler)))
		{
			LOG_ERROR("Failed to create Vulkan Image Sampler");
			return false;
		}
	}

	return true;
}

void VulkanImage::DestroyImage(CVulkanRenderer* pRenderingContext, VulkanImage* pImage)
{
	if (pImage->m_ImageSampler)
	{
		vkDestroySampler(pRenderingContext->m_Device.m_LogicalDevice, pImage->m_ImageSampler, pRenderingContext->m_Allocator);
	}

	if (pImage->m_ImageView)
	{
		vkDestroyImageView(pRenderingContext->m_Device.m_LogicalDevice, pImage->m_ImageView, pRenderingContext->m_Allocator);
	}

	if (pImage)
	{
		vkDestroyImage(pRenderingContext->m_Device.m_LogicalDevice, pImage->m_Handle, pRenderingContext->m_Allocator);
		vkFreeMemory(pRenderingContext->m_Device.m_LogicalDevice, pImage->m_ImageMemory, pRenderingContext->m_Allocator);
	}
}

bool VulkanImage::TransitionImageLayout(CVulkanRenderer* pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VulkanCommandBuffer commandBuffer;
	commandBuffer.BeginSingleUse(pRenderingContext, &pRenderingContext->m_GraphicsCommandPool.m_Handle);

	VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = m_Handle;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	// Image layout needs to transition to a transfer destination layout
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// Image layout needs to transition to a layout for shader reading
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		LOG_ERROR("TransitionImageLayout()->Unsupported image layout transition provided - Nothing was done.");
		return false;
	}

	vkCmdPipelineBarrier(commandBuffer.m_Handle, sourceStage, destinationStage,
						0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

	commandBuffer.EndSingleUse(pRenderingContext, &pRenderingContext->m_GraphicsCommandPool.m_Handle);
	return true;
}