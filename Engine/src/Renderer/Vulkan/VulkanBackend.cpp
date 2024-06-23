#include "VulkanBackend.h"

#include "Platform/VulkanPlatform.h"

#include "Core/Logging.h"

#include <Containers/CVector.h>
#include <String_Utils.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

bool CVulkanRenderer::RendererBackend_Initialize()
{
	// Instance
	if (!CreateVulkanInstance())
		return false;

	// Debug Messenger
	if (!CreateDebugMessenger())
		return false;

	// Device
	if (!VulkanDevice::CreateDevice(this, &m_Device))
		return false;


	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	DestroyDebugMessenger();

	vkDestroyInstance(m_Instance, m_Allocator);
}

bool CVulkanRenderer::CreateVulkanInstance()
{
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Engine Testbed";
	appInfo.applicationVersion = 0;	// TODO: Link with versiongen system?
	appInfo.pEngineName = "";		// TODO: Need engine name
	appInfo.engineVersion = 0;
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &appInfo;

	// Extensions
	CVector<const char*> requiredExtensions;
	requiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
	requiredExtensions.PushBack(Platform_GetVulkanExtensions());

	if (EnableValidationLayers())
	{
		requiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (!QueryInstanceExtensionSupport(requiredExtensions))
	{
		LOG_ERROR("Required Vulkan instance extensions were not found - Application cannot continue.");
		return false;
	}

	instanceInfo.enabledExtensionCount = requiredExtensions.GetCount();
	instanceInfo.ppEnabledExtensionNames = requiredExtensions.Data();

	// Validation Layers
	CVector<const char*> requiredValidationLayers;
	if (EnableValidationLayers())
	{
		requiredValidationLayers.PushBack("VK_LAYER_KHRONOS_validation");

		if (!QueryValidationLayerSupport(requiredValidationLayers))
		{
			LOG_ERROR("Required Vulkan validation layers were not found - Application cannot continue.");
			return false;
		}

		instanceInfo.enabledLayerCount = requiredValidationLayers.GetCount();
		instanceInfo.ppEnabledLayerNames = requiredValidationLayers.Data();
	}
	else
	{
		// No validation layers in release mode
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.ppEnabledLayerNames = nullptr;
	}

	if (!VK_CHECK(vkCreateInstance(&instanceInfo, m_Allocator, &m_Instance)))
		LOG_FATAL("Failed to create Vulkan Instance");

	if (EnableValidationLayers())
	{
		LOG_INFO("Enabled Vulkan Extensions:");
		for (int i = 0; i < requiredExtensions.GetCount(); i++)
		{
			LOG_DEBUG("%s", requiredExtensions[i]);
		}

		LOG_INFO("Enabled Validation Layers:");
		for (int i = 0; i < requiredValidationLayers.GetCount(); i++)
		{
			LOG_DEBUG("%s", requiredValidationLayers[i]);
		}
	}

	LOG_INFO("Vulkan Instance Created");
	return true;
}

bool CVulkanRenderer::QueryInstanceExtensionSupport(CVector<const char*>& requiredExtensions)
{
	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	CVector<VkExtensionProperties> instanceExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.Data());

	for (unsigned int i = 0; i < requiredExtensions.GetCount(); i++)
	{
		bool bFound = false;
		for (unsigned int j = 0; j < extensionCount; j++)
		{
			if (StringCompare(requiredExtensions[i], instanceExtensions[j].extensionName))
			{
				bFound = true;
			}
		}

		if (!bFound)
		{
			// If we get out here without finding a required extension, then quit out as we cannot continue.
			LOG_ERROR("QueryInstanceExtensionSupport() -> Unable to find support for extension %s", requiredExtensions[i]);
			return false;
		}
	}

	return true;
}

bool CVulkanRenderer::QueryValidationLayerSupport(CVector<const char*>& requiredValidationLayers)
{
	unsigned int layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	CVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());

	for (unsigned int i = 0; i < requiredValidationLayers.GetCount(); i++)
	{
		bool bFound = false;
		for (unsigned int j = 0; j < layerCount; j++)
		{
			if (StringCompare(requiredValidationLayers[i], availableLayers[j].layerName))
			{
				bFound = true;
			}
		}

		if (!bFound)
		{
			// If we get out here without finding a required extension, then quit out as we cannot continue.
			LOG_ERROR("QueryValidationLayerSupport() -> Unable to find support for %s", requiredValidationLayers[i]);
			return false;
		}
	}

	return true;
}

bool CVulkanRenderer::CreateDebugMessenger()
{
	if (!EnableValidationLayers())
	{
		// No Vulkan debug callbacks in release builds!
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallbackFunc;

	PFN_vkCreateDebugUtilsMessengerEXT createDebuggerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (createDebuggerFunc != nullptr)
	{
		if (!VK_CHECK(createDebuggerFunc(m_Instance, &createInfo, m_Allocator, &m_DebugMessenger)))
		{
			LOG_ERROR("Failed to create Vulkan Debug Messenger");
			return false;
		}
		else
		{
			LOG_INFO("Vulkan Debugger Created");
			return true;
		}
	}
	else
	{
		LOG_ERROR("Failed to create debug messenger (VK_ERROR_EXTENSION_NOT_PRESENT)");
		return false;
	}
}

void CVulkanRenderer::DestroyDebugMessenger()
{
	if (!EnableValidationLayers())
	{
		// No Vulkan debug callbacks in release builds!
		return;
	}

	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebuggerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

	if (destroyDebuggerFunc != nullptr)
	{
		destroyDebuggerFunc(m_Instance, m_DebugMessenger, m_Allocator);
		return;
	}

	LOG_ERROR("Failed to destory Vulkan Debug Messenger");
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG_VERBOSE(pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG_INFO(pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG_WARN(pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_ERROR(pCallbackData->pMessage);
		break;
	}

	return VK_FALSE;
}

static CVulkanRenderer* s_VulkanRenderer{ nullptr };

IRendererBackend* GetVulkanBackend()
{
	if (!s_VulkanRenderer)
	{
		s_VulkanRenderer = new CVulkanRenderer();
		return s_VulkanRenderer;
	}
	else
	{
		return s_VulkanRenderer;
	}
}

