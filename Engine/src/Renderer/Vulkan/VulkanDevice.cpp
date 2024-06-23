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

bool VulkanDevice::CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice)
{
	if (!pOutDevice->SelectPhysicalDevice(pRenderingContext))
		return false;

	LOG_DEBUG("Selected GPU: %s", pOutDevice->m_PhysicalDeviceProperties.deviceName);

	return true;
}

bool VulkanDevice::SelectPhysicalDevice(CVulkanRenderer* pRenderingContext)
{
	unsigned int deviceCount = 0;
	vkEnumeratePhysicalDevices(pRenderingContext->GetInstance(), &deviceCount, nullptr);
	CVector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(pRenderingContext->GetInstance(), &deviceCount, devices.Data());


	VkPhysicalDevice* pDevice = nullptr;

	// Select the first device that is a discrete GPU
	for (unsigned int i = 0; i < devices.GetCount(); i++)
	{
		VkPhysicalDevice* currDevice = &devices[i];
		VkPhysicalDeviceProperties properties;
		
		vkGetPhysicalDeviceProperties(*currDevice, &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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

	return true;
}
