//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: Interface for Vulkan Devices
//=============================================================================
#include "VulkanBackend.h"

#include "Core/Logging.h"
#include "Core/Assertions.h"
#include <Containers/CVector.h>
#include <String_Utils.h>
#include <stdint.h>

#include <Containers/CSet.h>


bool VulkanDevice::CreateVulkanDevice(CVulkanRenderer* renderingContext)
{
	if (!SelectPhysicalDevice(renderingContext))
		return false;

	GetQueueFamilyData(renderingContext);

	// Query the device's swapchain support and pass that information to the swapchain itself.
	QueryDeviceSwapchainSupport(renderingContext, m_PhysicalDeviceHandle, renderingContext->GetSwapchain());

	CreateLogicalDevice(renderingContext);

	return true;
}

void VulkanDevice::DestroyLogicalDevice(CVulkanRenderer* renderingContext)
{
	// TODO: Custom Allocator
	vkDestroyDevice(GetLogicalDevice(), nullptr);
}

void VulkanDevice::QueryDeviceSwapchainSupport(CVulkanRenderer* renderingContext, VkPhysicalDevice device, VulkanSwapchain& swapchain)
{
	// Get a handle to the (yet-to-be-created) swapchain to hand it the data directly.
	SwapchainSupportDetails_t& swapchainSupportDetails = swapchain.GetSwapchainSupportDetails();

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, renderingContext->GetVulkanSurface(), &swapchainSupportDetails.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderingContext->GetVulkanSurface(), &formatCount, nullptr);

	if (formatCount != 0)
	{
		swapchainSupportDetails.formats.Resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderingContext->GetVulkanSurface(), &formatCount, swapchainSupportDetails.formats.Data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderingContext->GetVulkanSurface(), &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		swapchainSupportDetails.presentModes.Resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderingContext->GetVulkanSurface(), &presentModeCount, swapchainSupportDetails.presentModes.Data());
	}

	// Flip the 'supports swapchain' flag to true if this is the case
	m_bDeviceSupportsSwapchain = (!(swapchainSupportDetails.formats.Empty() && swapchainSupportDetails.presentModes.Empty()));
}

bool VulkanDevice::SelectPhysicalDevice(CVulkanRenderer* renderingContext)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(renderingContext->GetVulkanInstance(), &physicalDeviceCount, nullptr);
	CVector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(renderingContext->GetVulkanInstance(), &physicalDeviceCount, physicalDevices.Data());

	for (int i = 0; i < physicalDevices.GetCount(); i++)
	{
		if (PhysicalDeviceMeetsRequirements(renderingContext, physicalDevices[i]))
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

bool VulkanDevice::PhysicalDeviceMeetsRequirements(CVulkanRenderer* renderingContext, VkPhysicalDevice& physicalDevice)
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

void VulkanDevice::GetQueueFamilyData(CVulkanRenderer* renderingContext)
{
	uint32_t numOfDeviceQueueFamiliesAvailable = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &numOfDeviceQueueFamiliesAvailable, nullptr);
	CVector<VkQueueFamilyProperties> deviceQueueFamilyProperties(numOfDeviceQueueFamiliesAvailable);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &numOfDeviceQueueFamiliesAvailable, deviceQueueFamilyProperties.Data());

	VkBool32 presentSupport = false;
	for (int i = 0; i < deviceQueueFamilyProperties.GetCount(); i++)
	{
		// We query support for the present queue index first as it is very possible that this queue is shared with another task
		// Putting this above the rest of the loop allows it to be run only once, as well as allows the other 'if' checks to continue
		// after finding an index without worrying about the present queue.
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDeviceHandle, i, renderingContext->GetVulkanSurface(), &presentSupport);

		if (presentSupport)
		{
			m_PhysicalDeviceQueueIndices.presentIndex = i;
		}

		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_PhysicalDeviceQueueIndices.graphicsIndex = i;
			continue;
		}

		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			m_PhysicalDeviceQueueIndices.computeIndex = i;
			continue;
		}

		if (deviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			m_PhysicalDeviceQueueIndices.transferIndex = i;
			continue;
		}

		if (m_PhysicalDeviceQueueIndices.IsComplete())
		{
			break;
		}
	}
}

bool VulkanDevice::QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice)
{
	uint32_t deviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
	CVector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.Data());

	int extensionsToQuery = m_DeviceExtensions.GetCount();
	
	for (int i = 0; i < m_DeviceExtensions.GetCount(); i++)
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

		if (extensionsToQuery == 0)
			break;
	}

#if _DEBUG
	LOG_VERBOSE("Enabled Vulkan Device Extensions:");
	for (int i = 0; i < m_DeviceExtensions.GetCount(); i++)
	{
		LOG_VERBOSE("%s", m_DeviceExtensions[i]);
	}
#endif

	return extensionsToQuery == 0;
}

bool VulkanDevice::CreateLogicalDevice(CVulkanRenderer* renderingContext)
{
	DeviceQueueFamilyProperties_t& queueIndices = m_PhysicalDeviceQueueIndices;

	CVector<VkDeviceQueueCreateInfo> queueCreateInfos;
	CSet<int> uniqueQueueFamilies{ (int)queueIndices.graphicsIndex, (int)queueIndices.presentIndex, (int)queueIndices.transferIndex, (int)queueIndices.computeIndex };

	// Set up the device queues
	float queuePriority = 1.0f;
	for (int i = 0; i < uniqueQueueFamilies.GetSize(); i++)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.PushBack(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.GetCount());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.Data();

	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.GetCount());
	deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.Data();

	if (renderingContext->EnableValidationLayers())
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(renderingContext->GetValidationLayers().GetCount());
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
	vkGetDeviceQueue(m_LogicalDeviceHandle, queueIndices.presentIndex, 0, &m_PresentQueue);
	vkGetDeviceQueue(m_LogicalDeviceHandle, queueIndices.transferIndex, 0, &m_TransferQueue);
	vkGetDeviceQueue(m_LogicalDeviceHandle, queueIndices.computeIndex, 0, &m_PresentQueue);

	LOG_DEBUG("Graphics Queue Index: %d", m_PhysicalDeviceQueueIndices.graphicsIndex);
	LOG_DEBUG("Present Queue Index: %d", m_PhysicalDeviceQueueIndices.presentIndex);
	LOG_DEBUG("Transfer Queue Index: %d", m_PhysicalDeviceQueueIndices.transferIndex);
	LOG_DEBUG("Compute Queue Index: %d", m_PhysicalDeviceQueueIndices.computeIndex);

	return true;
}

bool VulkanDevice::CreateCommandBuffers(CVulkanRenderer* renderingContext)
{
	// Create the graphics command buffer pool.
	VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	// Allow us to reset our allocated command buffers.
	graphicsCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsCommandPoolCreateInfo.queueFamilyIndex = m_PhysicalDeviceQueueIndices.graphicsIndex;

	// TODO: Custom allocator
	VkResult result = vkCreateCommandPool(m_LogicalDeviceHandle, &graphicsCommandPoolCreateInfo, nullptr, &m_GraphicsCommandPool);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create graphics command pool!");
		return false;
	}

	// Allocate our command buffers (one per concurrent frame)
	VkCommandBufferAllocateInfo commandBufferAllocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocInfo.commandPool = m_GraphicsCommandPool;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // TODO: This should probably be configurable.
	commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	result = vkAllocateCommandBuffers(m_LogicalDeviceHandle, &commandBufferAllocInfo, m_GraphicsCommandBuffers);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocate graphics command buffers!");
		return false;
	}

	LOG_INFO("Command Buffers Allocated");
	return true;
}

void VulkanDevice::DestroyCommandBuffers(CVulkanRenderer* renderingContext)
{
	// TODO: Custom allocator
	vkFreeCommandBuffers(m_LogicalDeviceHandle, m_GraphicsCommandPool, MAX_FRAMES_IN_FLIGHT, m_GraphicsCommandBuffers);
	vkDestroyCommandPool(m_LogicalDeviceHandle, m_GraphicsCommandPool, nullptr);
}

bool VulkanDevice::BeginCommandBuffer(VkCommandBuffer& commandBuffer)
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);

	if (beginResult != VK_SUCCESS)
	{
		LOG_ERROR("BeginCommandBuffer()->Failed to start recording");
		return false;
	}

	return true;
}

bool VulkanDevice::EndCommandBuffer(VkCommandBuffer& commandBuffer)
{
	VkResult endResult = vkEndCommandBuffer(commandBuffer);

	if (endResult != VK_SUCCESS)
	{
		LOG_ERROR("BeginCommandBuffer()->Failed to start recording");
		return false;
	}

	return true;
}

bool VulkanDevice::SubmitCommandBuffer(VkCommandBuffer& commandBuffer)
{
	// TODO: These will need to hook up with our semaphores/fences soon.
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = 0;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	if (!vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, nullptr))
	{
		LOG_ERROR("Command Buffer failed to submit!");
		return false;
	}

	return true;
}

