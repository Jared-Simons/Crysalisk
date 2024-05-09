//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: Interface for Vulkan Devices
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"
#include <CVector.h>
#include <String_Utils.h>
#include <stdint.h>

bool VulkanDevice_t::CreateVulkanDevice(CVulkanRenderer* renderingContext)
{
	LOG_DEBUG("Creating Vulkan Device");

	if (!SelectPhysicalDevice(renderingContext))
		return false;

	GetQueueFamilyData();

	CreateLogicalDevice(renderingContext);

	return true;
}

void VulkanDevice_t::DestroyLogicalDevice(CVulkanRenderer* renderingContext)
{
	// TODO: Custom Allocator
	vkDestroyDevice(GetLogicalDevice(), nullptr);
}

bool VulkanDevice_t::SelectPhysicalDevice(CVulkanRenderer* renderingContext)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(renderingContext->GetVulkanInstance(), &physicalDeviceCount, nullptr);
	CVector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(renderingContext->GetVulkanInstance(), &physicalDeviceCount, physicalDevices.Data());

	for (int i = 0; i < physicalDevices.GetSize(); i++)
	{
		if (PhysicalDeviceMeetsRequirements(physicalDevices[i]))
		{
			break;
		}
	}

	if (m_PhysicalDeviceHandle == VK_NULL_HANDLE)
	{
		LOG_FATAL("Unable to find a physical graphics device, application cannot continue.");
		return false;
	}
	else
	{
		LOG_DEBUG("Selected Graphics Device: %s", m_PhysicalDeviceProperties.deviceName);
		return true;
	}
}

bool VulkanDevice_t::PhysicalDeviceMeetsRequirements(VkPhysicalDevice& physicalDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	bool deviceHasRequiredExtensions = QueryDeviceExtensionSupport(physicalDevice);


	// Select the first physical device that meets the requirements set here.
	// Currently the only requirements are a discrete GPU and that it has access to the swapchain extension.
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceHasRequiredExtensions)
	{
		// Save off attributes about the device.
		m_PhysicalDeviceProperties = deviceProperties;

		// Save off a handle to the device.
		m_PhysicalDeviceHandle = physicalDevice;
		return true;
	}

	return false;
}

void VulkanDevice_t::GetQueueFamilyData()
{
	uint32_t numOfDeviceQueueFamiliesAvailable = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &numOfDeviceQueueFamiliesAvailable, nullptr);
	CVector<VkQueueFamilyProperties> deviceQueueFamilyProperties(numOfDeviceQueueFamiliesAvailable);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &numOfDeviceQueueFamiliesAvailable, deviceQueueFamilyProperties.Data());

	for (int i = 0; i < deviceQueueFamilyProperties.GetSize(); i++)
	{
		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && m_PhysicalDeviceQueueFamilyProperties.graphicsIndex == -1)
		{
			m_PhysicalDeviceQueueFamilyProperties.graphicsIndex = i;
			continue;
		}

		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT && m_PhysicalDeviceQueueFamilyProperties.computeIndex == -1)
		{
			m_PhysicalDeviceQueueFamilyProperties.computeIndex = i;
			continue;
		}

		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && m_PhysicalDeviceQueueFamilyProperties.transferIndex == -1)
		{
			m_PhysicalDeviceQueueFamilyProperties.transferIndex = i;
			continue;
		}
	}
}

bool VulkanDevice_t::QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice)
{
	uint32_t deviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
	CVector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.Data());

	int extensionsToQuery = m_DeviceExtensions.GetSize();
	
	for (int i = 0; i < m_DeviceExtensions.GetSize(); i++)
	{
		for (int j = 0; j < deviceExtensionCount; j++)
		{
			if (StringCompare(availableDeviceExtensions[j].extensionName, m_DeviceExtensions[i]) == true)
			{
				// Found the current extension
				extensionsToQuery--;
				break;
			}
		}
	}
	
	return extensionsToQuery == 0;

}

bool VulkanDevice_t::CreateLogicalDevice(CVulkanRenderer* renderingContext)
{
	DeviceQueueFamilyProperties_t& queueIndices = m_PhysicalDeviceQueueFamilyProperties;

	CVector<VkDeviceQueueCreateInfo> queueCreateInfos;
	uint32_t uniqueQueueFamilies[1] = { queueIndices.graphicsIndex };

	// Required Device Extensions
	CVector<const char*> requiredDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// Set up the device queues
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.PushBack(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.GetSize());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.Data();

	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.GetSize());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.Data();

	if (renderingContext->EnableValidationLayers())
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(renderingContext->GetValidationLayers().GetSize());
		deviceCreateInfo.ppEnabledLayerNames = renderingContext->GetValidationLayers().Data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	// TODO: Custom Allocator
	if (vkCreateDevice(m_PhysicalDeviceHandle, &deviceCreateInfo, nullptr, &m_LogicalDeviceHandle) != VK_SUCCESS)
	{
		AssertMsg("Failed to create logical device!");
	}

	LOG_INFO("Vulkan Logical Device created.");

	vkGetDeviceQueue(m_LogicalDeviceHandle, queueIndices.graphicsIndex, 0, &m_GraphicsQueue);

	LOG_DEBUG("Graphics Queue Index: %d", m_PhysicalDeviceQueueFamilyProperties.graphicsIndex);

	return true;
}

