//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2025
// 
// Purpose: Methods and utilities for Vulkan Images
//=============================================================================
#include "VulkanRenderer.h"

bool VulkanImage::CreateImage(CVulkanRenderer& pRenderingContext, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VulkanImage& pOutImage)
{
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = imageFormat;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1; // No mips for now
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usageFlags;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateImage(pRenderingContext.m_Device.m_Device, &imageInfo, pRenderingContext.m_Allocator, &pOutImage.m_Image));

	// Allocate memory for the image.
	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(pRenderingContext.m_Device.m_Device, pOutImage.m_Image, &imageMemoryRequirements);

	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = imageMemoryRequirements.size;
	memAllocInfo.memoryTypeIndex = pRenderingContext.m_Device.FindMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vkAllocateMemory(pRenderingContext.m_Device.m_Device, &memAllocInfo, pRenderingContext.m_Allocator, &pOutImage.m_Memory));

	vkBindImageMemory(pRenderingContext.m_Device.m_Device, pOutImage.m_Image, pOutImage.m_Memory, 0);

	// Create Image View
	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.image = pOutImage.m_Image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = imageInfo.format; // TODO: Configurable
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(pRenderingContext.m_Device.m_Device, &viewCreateInfo, pRenderingContext.m_Allocator, &pOutImage.m_View));

	return true;
}

void VulkanImage::DestroyImage(CVulkanRenderer& pRenderingContext, VulkanImage& pImage)
{
	vkDestroyImageView(pRenderingContext.m_Device.m_Device, pImage.m_View, pRenderingContext.m_Allocator);
	vkFreeMemory(pRenderingContext.m_Device.m_Device, pImage.m_Memory, pRenderingContext.m_Allocator);
	vkDestroyImage(pRenderingContext.m_Device.m_Device, pImage.m_Image, pRenderingContext.m_Allocator);
}

bool VulkanImage::TransitionImageLayout(class CVulkanRenderer& pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VulkanCommandBuffer commandBuffer{};
	commandBuffer.CreateAndBeginSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);

	VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = m_Image;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage{};
	VkPipelineStageFlags destStage{};

	// Image layout needs to transition to a transfer destination layout
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// Image layout needs to transition to be optimal for shader sampling.
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		LOG_ERROR("TransitionImageLayout()->Unsupported image layout transition provided - Nothing was done.");
		return false;
	}

	vkCmdPipelineBarrier(commandBuffer.m_Handle, sourceStage, destStage,
						0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

	commandBuffer.EndSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);

	return true;
}

bool VulkanImage::WriteDataToImage(CVulkanRenderer& pRenderingContext, uint32_t width, uint32_t height, uint8_t numChannels, byte* pPixelData)
{
	// Put the image data into a nice staging buffer.
	VulkanBuffer stagingBuffer;
	VulkanBuffer::CreateBuffer(pRenderingContext, width * height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer);

	void* pImageData;
	vkMapMemory(pRenderingContext.m_Device.m_Device, stagingBuffer.m_Memory, 0, width * height * numChannels, 0, &pImageData);
	memcpy(pImageData, pPixelData, width * height * numChannels);
	vkUnmapMemory(pRenderingContext.m_Device.m_Device, stagingBuffer.m_Memory);

	// Transition the image to a transfer dest. layout
	if (!TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
	{
		LOG_ERROR("Failed to transition image layout for data upload.");
		return false;
	}

	// Copy the buffer data to the image.
	// FIXME: Make this it's own method?
	{
		VulkanCommandBuffer cmdBuffer{};
		cmdBuffer.CreateAndBeginSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(cmdBuffer.m_Handle, stagingBuffer.m_Handle, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		cmdBuffer.EndSingleUse(pRenderingContext, pRenderingContext.m_GraphicsCommandPool);
	}

	// Transition the image to a layout optimal for reading in the shader.
	if (!TransitionImageLayout(pRenderingContext, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
	{
		LOG_ERROR("Failed to transition image layout for sampling.");
		return false;
	}

	// Release the staging buffer.
	VulkanBuffer::DestroyBuffer(pRenderingContext, stagingBuffer);

	return true;
}