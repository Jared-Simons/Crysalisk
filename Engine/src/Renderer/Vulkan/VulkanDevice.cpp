//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: Interface for Vulkan Devices
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"

#include <Containers/CSet.h>
#include <String_Utils.h>

bool VulkanDevice::CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice)
{
	// Setup required device extensions
	pOutDevice->m_DeviceExtensions = new CVector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	if (!pOutDevice->SelectPhysicalDevice(pRenderingContext))
		return false;

	if (!pOutDevice->CreateLogicalDevice(pRenderingContext))
		return false;

	LOG_DEBUG("Selected GPU: '%s'", pOutDevice->m_PhysicalDeviceProperties.deviceName);
	LOG_DEBUG("Graphics Queue Index:\t%d", pOutDevice->m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex);
	LOG_DEBUG("Present Queue Index:\t%d", pOutDevice->m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex);
	LOG_DEBUG("Transfer Queue Index:\t%d", pOutDevice->m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex);
	LOG_DEBUG("Compute Queue Index:\t%d", pOutDevice->m_PhysicalDeviceQueueFamilyInfo.ComputeQueueIndex);

	return true;
}

void VulkanDevice::DestroyDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pDevice)
{
	if (pDevice && pDevice->m_LogicalDevice)
	{
		delete pDevice->m_DeviceExtensions;
		vkDestroyDevice(pDevice->m_LogicalDevice, pRenderingContext->m_Allocator);
	}
}

bool VulkanDevice::SelectPhysicalDevice(CVulkanRenderer* pRenderingContext)
{
	unsigned int deviceCount{ 0 };
	vkEnumeratePhysicalDevices(pRenderingContext->m_Instance, &deviceCount, nullptr);
	CVector<VkPhysicalDevice> devices(deviceCount, true);
	vkEnumeratePhysicalDevices(pRenderingContext->m_Instance, &deviceCount, devices.data());


	VkPhysicalDevice* pDevice = nullptr;

	// Select the first device that is a discrete GPU
	for (unsigned int i = 0; i < devices.size(); i++)
	{
		VkPhysicalDevice* currDevice = &devices[i];
		VkPhysicalDeviceProperties properties;
		
		vkGetPhysicalDeviceProperties(*currDevice, &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && QueryDeviceExtensionSupport(*currDevice))
		{
			pDevice = currDevice;
			break;
		}
	}

	// If we get here with no device, then none was suitable. Exit out here.
	if (!pDevice)
	{
		LOG_ERROR("Failed to find Discrete GPU with Vulkan support");
		return false;
	}

	// Save off the selected device and it's properties.
	m_PhysicalDevice = *pDevice;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);

	// Get the device's queue family properties.
	if (!QueryDeviceQueueFamilyProperties(pRenderingContext, m_PhysicalDevice, &m_PhysicalDeviceQueueFamilyInfo))
	{
		LOG_ERROR("Failed to find Queue Family Properties for selected device.");
		return false;
	}

	return true;
}

bool VulkanDevice::QueryDeviceQueueFamilyProperties(CVulkanRenderer* pRenderingContext, VkPhysicalDevice& physicalDevice, DeviceQueueFamilyInfo_t* pOutQueueInfo)
{
	if (!physicalDevice)
	{
		LOG_ERROR("QueryDeviceQueueFamilyProperties() -> physicalDevice was nullptr: device setup cannot continue.");
		return false;
	}

	unsigned int queueFamilyPropertyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
	CVector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount, true);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	for (unsigned int i = 0; i < queueFamilyProperties.size(); i++)
	{
		// Look for a graphics queue.
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			pOutQueueInfo->GraphicsQueueIndex = i;
		}

		// Look for a compute queue.
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
		{
			pOutQueueInfo->ComputeQueueIndex = i;
		}

		// Look for a transfer queue.
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
		{
			pOutQueueInfo->TransferQueueIndex = i;
		}

		// Look for a present queue.
		VkBool32 queueSupportsPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, pRenderingContext->m_Surface, &queueSupportsPresent);

		if (queueSupportsPresent)
		{
			pOutQueueInfo->PresentQueueIndex = i;
		}
	}


	if (!pOutQueueInfo->IsComplete())
	{
		LOG_ERROR("QueryDeviceQueueFamilyProperties() -> Could not find necessary queue families - device setup cannot continue.");
		return false;
	}

	return true;
}

bool VulkanDevice::QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice)
{
	// Check for required device extensions.
	// Currently we are only looking for the swapchain extension.

	unsigned int deviceExtensionCount{ 0 };
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
	CVector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());

	CVector<const char*> requiredExtensions = *m_DeviceExtensions;

	int extensionsToQuery = requiredExtensions.size();
	for (int i = 0; i < requiredExtensions.size(); i++)
	{
		for (int j = 0; j < deviceExtensionCount; j++)
		{
			if (StringCompare(availableDeviceExtensions[j].extensionName, requiredExtensions[i]) == true)
			{
				// Found the current extension
				extensionsToQuery--;
				break;
			}
		}

		if (extensionsToQuery == 0)
			break;
	}

#if _DEBUG
	LOG_VERBOSE("Enabled Vulkan Device Extensions:");
	for (int i = 0; i < requiredExtensions.size(); i++)
	{
		LOG_VERBOSE("%s", requiredExtensions[i]);
	}
#endif

	return extensionsToQuery == 0;
}

bool VulkanDevice::CreateLogicalDevice(CVulkanRenderer* pRenderingContext)
{
	const float queuePriority{ 1.0f };

	CVector<VkDeviceQueueCreateInfo> queueCreateInfos;
	CSet<int> uniqueQueueFamilies{ m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex, m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex, m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex, m_PhysicalDeviceQueueFamilyInfo.ComputeQueueIndex };

	for (int i = 0; i < uniqueQueueFamilies.GetSize(); i++)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.PushBack(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions->size();
	deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions->data();
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.pEnabledFeatures = &m_PhysicalDeviceFeatures;

	if (!VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, pRenderingContext->m_Allocator, &m_LogicalDevice)))
	{
		LOG_ERROR("Failed to create logical device.");
		return false;
	}

	LOG_INFO("Vulkan device created");

	vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.GraphicsQueueIndex, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.PresentQueueIndex, 0, &m_PresentQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.TransferQueueIndex, 0, &m_TransferQueue);
	vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDeviceQueueFamilyInfo.ComputeQueueIndex, 0, &m_ComputeQueue);

	return true;
}

void VulkanDevice::QuerySwapchainSupportDetails(CVulkanRenderer* pRenderingContext, SwapchainSupportDetails* pOutDetails)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, pRenderingContext->m_Surface, &pOutDetails->surfaceCapabilities);

	unsigned int numFormats{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, pRenderingContext->m_Surface, &numFormats, nullptr);
	pOutDetails->formats.Resize(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, pRenderingContext->m_Surface, &numFormats, pOutDetails->formats.data());

	unsigned int numPresentModes{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, pRenderingContext->m_Surface, &numPresentModes, nullptr);
	pOutDetails->presentModes.Resize(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, pRenderingContext->m_Surface, &numPresentModes, pOutDetails->presentModes.data());

}
