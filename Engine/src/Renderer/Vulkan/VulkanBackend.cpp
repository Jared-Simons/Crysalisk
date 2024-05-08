//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#include "VulkanBackend.h"
#include "Platform/VulkanPlatform.h"

#include "Core/Assertions.h"

#include <String_Utils.h>
#include <stdint.h>

IRendererBackend* GetVulkanBackend()
{
	return &VulkanRenderingContext;
}

bool CVulkanBackend::RendererBackend_Initialize()
{
	CreateVulkanInstance();

	LOG_DEBUG("Vulkan Rendering Backend Initialized");
	return true;
}

void CVulkanBackend::RendererBackend_Shutdown()
{
	LOG_DEBUG("Vulkan Rendering Backend Shutting Down");

	// Shutdown renderer components in reverse order.
	// TODO: Custom allocator.

	vkDestroyInstance(m_Instance, nullptr);
}

bool CVulkanBackend::RendererBackend_Update()
{
	return true;
}

bool CVulkanBackend::CreateVulkanInstance()
{
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	// TODO: Pass these in from the frontend
	appInfo.pApplicationName = "NULL";
	appInfo.pEngineName = "NULL";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	// TODO: Link these with a versiongen system.
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	// Validation Layers
	if (EnableValidationLayers())
	{
		if (!RequestValidationLayerSupport())
		{
			AssertMsg("Validation layers requested, but not available!");
		}

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.GetNumElements());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.Data();

		LOG_INFO("Enabled Vulkan Validation Layers: ");
		for (int i = 0; i < m_ValidationLayers.GetNumElements(); i++)
		{
			LOG_DEBUG(m_ValidationLayers[i]);
		}
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	// Extensions
	CVector<const char*> extensionNames;
	extensionNames.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionNames.PushBack(Platform_GetVulkanExtensions());

#if _DEBUG
	extensionNames.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	createInfo.enabledExtensionCount = extensionNames.GetNumElements();
	createInfo.ppEnabledExtensionNames = extensionNames.Data();

#if _DEBUG
	LOG_INFO("Enabled Vulkan Extensions: ");
	for (int i = 0; i < extensionNames.GetNumElements(); i++)
	{
		LOG_DEBUG(extensionNames[i]);
	}
#endif

	// TODO: Custom allocator
	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		AssertMsg("Failed to create Vulkan Instance!");
		return false;
	}

	LOG_DEBUG("Vulkan Instance Created");
	return true;
}

bool CVulkanBackend::RequestValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	CVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());

	for (int i = 0; i < m_ValidationLayers.GetNumElements(); i++)
	{
		bool layerFound = false;
		for (int j = 0; i < availableLayers.GetNumElements(); j++)
		{
			if (StringCompare(m_ValidationLayers[i], availableLayers[j].layerName) == true)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

