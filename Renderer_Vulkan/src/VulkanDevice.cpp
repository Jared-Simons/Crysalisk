//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: interface for Vulkan Devices
//=============================================================================
#include "VulkanRenderer.h"

#include <Strings/String_Utils.h>

static bool QueryDeviceExtensionSupport(VkPhysicalDevice pDevice);

bool VulkanDevice::CreateDevice(CVulkanRenderer& pRenderingContext, VulkanDevice& pOutDevice)
{
	// Select physical device.
	// TODO: Pass in a configuration from the front-end to query device features.

	unsigned int physicalDeviceCount{ 0 };
	vkEnumeratePhysicalDevices(pRenderingContext.m_Instance, &physicalDeviceCount, nullptr);
	Utl::CVector<VkPhysicalDevice> availableDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(pRenderingContext.m_Instance, &physicalDeviceCount, availableDevices.Data());

	// For now, just grab the first discrete GPU.
	// TODO: Perform more fine-grained selection based on requirements.
	VkPhysicalDevice selectedDevice{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties properties{};
	for (unsigned int deviceIndex = 0; deviceIndex < physicalDeviceCount; deviceIndex++)
	{
		vkGetPhysicalDeviceProperties(availableDevices[deviceIndex], &properties);
		
		if (properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && QueryDeviceExtensionSupport(availableDevices[deviceIndex]))
		{
			// This one is good!
			selectedDevice = availableDevices[deviceIndex];
			break;
		}
	}

	if (selectedDevice == VK_NULL_HANDLE)
	{
		LOG_ERROR("Failed to find a suitable GPU for rendering!");
		return false;
	}
	else
	{
		// Cache off the selected device.
		LOG_DEBUG("Selected GPU: %s", properties.deviceName);
		pOutDevice.m_PhysicalDevice = selectedDevice;
	}

	// Query physical device queue information.
	unsigned int deviceQueueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &deviceQueueFamilyCount, nullptr);
	Utl::CVector<VkQueueFamilyProperties> queueFamilyProperties(deviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &deviceQueueFamilyCount, queueFamilyProperties.Data());

	for (unsigned int queueFamilyIndex = 0; queueFamilyIndex < deviceQueueFamilyCount; queueFamilyIndex++)
	{
		if ((queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex < 1)) // Don't change indices if this has already been assigned.
		{
			// Found an index for graphics.
			pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex = queueFamilyIndex;
		}

		// Check for present support as well
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(pOutDevice.m_PhysicalDevice, queueFamilyIndex, pRenderingContext.m_Surface, &presentSupport);

		if (presentSupport && (pOutDevice.m_QueueFamilyIndices.PresentQueueFamilyIndex < 1))
		{
			pOutDevice.m_QueueFamilyIndices.PresentQueueFamilyIndex = queueFamilyIndex;
		}
	}

	// A possible graphics queue is required
	if (pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex < 0)
	{
		// TODO: Try to select another device maybe?
		LOG_ERROR("Failed to find a possible graphics queue on selected physical device. Renderer initialization failed.");
		return false;
	}

	Utl::CVector<const char*> requiredExtensions(1);
	requiredExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	const float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfos[2];	// [0] = Graphics queue, [1] = Present queue
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &queuePriority;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].pNext = nullptr;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = pOutDevice.m_QueueFamilyIndices.PresentQueueFamilyIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = &queuePriority;
	queueCreateInfos[1].flags = 0;
	queueCreateInfos[1].pNext = nullptr;

	// Setup logical device.
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 2;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.enabledExtensionCount = static_cast<unsigned int>(requiredExtensions.Count());
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.Data();

	VK_CHECK(vkCreateDevice(pOutDevice.m_PhysicalDevice, &deviceCreateInfo, pRenderingContext.m_Allocator, &pOutDevice.m_Device));
	LOG_DEBUG("Vulkan logical device created");

	// Obtain queue handles.
	vkGetDeviceQueue(pOutDevice.m_Device, pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex, 0, &pOutDevice.m_GraphicsQueue);
	vkGetDeviceQueue(pOutDevice.m_Device, pOutDevice.m_QueueFamilyIndices.PresentQueueFamilyIndex, 0, &pOutDevice.m_PresentQueue);

	LOG_DEBUG("Graphics Queue Index:\t%d", pOutDevice.m_QueueFamilyIndices.GraphicsQueueFamilyIndex);
	LOG_DEBUG("Present Queue Index:\t%d", pOutDevice.m_QueueFamilyIndices.PresentQueueFamilyIndex);

	// Cache off appropriate device features.
	vkGetPhysicalDeviceMemoryProperties(pOutDevice.m_PhysicalDevice, &pOutDevice.m_MemoryProperties);
	vkGetPhysicalDeviceFeatures(pOutDevice.m_PhysicalDevice, &pOutDevice.m_DeviceFeatures);
	vkGetPhysicalDeviceProperties(pOutDevice.m_PhysicalDevice, &pOutDevice.m_DeviceProperties);

	return true;
}

void VulkanDevice::DestroyDevice(CVulkanRenderer& pRenderingContext, VulkanDevice& pDevice)
{
	vkDestroyDevice(pDevice.m_Device, pRenderingContext.m_Allocator);
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t typeIndex = 0; typeIndex < m_MemoryProperties.memoryTypeCount; typeIndex++)
	{
		if ((typeFilter & (1 << typeIndex)) && (m_MemoryProperties.memoryTypes[typeIndex].propertyFlags & properties) == properties)
		{
			return typeIndex;
		}
	}

	CASSERTMSG(0, "Failed to find suitable memory type for Vulkan operation!");
	return 0;
}

VkFormat VulkanDevice::FindSupportedDepthFormat()
{
	// TODO: make this work when checking out device candidates, not just after selecting a device.
	// TODO: make this more configurable.
	Utl::CVector<VkFormat> formatCandidates{};
	formatCandidates.PushBack(VK_FORMAT_D32_SFLOAT);
	formatCandidates.PushBack(VK_FORMAT_D32_SFLOAT_S8_UINT);
	formatCandidates.PushBack(VK_FORMAT_D24_UNORM_S8_UINT);

	for (int idx = 0; idx < formatCandidates.Count(); idx++)
	{
		VkFormatProperties formatProps{};
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, formatCandidates[idx], &formatProps);

		if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return formatCandidates[idx];
		}
	}

	return VK_FORMAT_UNDEFINED;
}

static bool QueryDeviceExtensionSupport(VkPhysicalDevice pDevice)
{
	// Check for required device extensions.
	// Currently we are only looking for the swapchain extension.
	unsigned int deviceExtensionCount{ 0 };
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &deviceExtensionCount, nullptr);
	Utl::CVector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &deviceExtensionCount, availableExtensions.Data());

	Utl::CVector<const char*> requiredExtensions(1);
	requiredExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	int extensionsToQuery = static_cast<uint32_t>(requiredExtensions.Count());
	for (int i = 0; i < requiredExtensions.Count(); i++)
	{
		LOG_VERBOSE("Searching for device extension: '%s'...", requiredExtensions[i]);
		for (unsigned int j = 0; j < deviceExtensionCount; j++)
		{
			if (StringCompare(availableExtensions[j].extensionName, requiredExtensions[i]))
			{
				// Found the current extension
				LOG_VERBOSE("Found!");
				extensionsToQuery--;
				break;
			}
		}

		if (extensionsToQuery == 0)
			break;
	}

	return extensionsToQuery == 0;
}