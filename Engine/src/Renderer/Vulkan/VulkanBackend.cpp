//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#include "VulkanBackend.h"
#include "Platform/VulkanPlatform.h"
#include "Systems/IMemory.h"

#include "Core/Assertions.h"

#include <String_Utils.h>
#include <stdint.h>


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);


IRendererBackend* GetVulkanBackend()
{
	return &VulkanRenderingContext;
}

bool CVulkanRenderer::RendererBackend_Initialize()
{
	if (!CreateVulkanInstance())
		return false;

	if (!CreateDebugMessenger())
		return false;

	if (!Platform_CreateVulkanSurface(this))
		return false;

	if (!m_Device.CreateVulkanDevice(this))
		return false;

	if (!m_Swapchain.CreateSwapchain(this))
		return false;

	if (!m_Swapchain.CreateImageViews(this))
		return false;

	if (!m_MainRenderpass.CreateRenderpass(this))
		return false;

	if (!m_Device.CreateCommandBuffers(this))
		return false;

	LOG_INFO("Vulkan Rendering Backend Initialized");
	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	LOG_INFO("Vulkan Rendering Backend Shutting Down");
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());

	// Shutdown renderer components in reverse order.
	// TODO: Custom allocator.

	m_Device.DestroyCommandBuffers(this);
	m_MainRenderpass.DestroyRenderpass(this);
	m_Swapchain.DestroyImageViews(this);
	m_Swapchain.CleanupSwapchain(this);
	m_Device.DestroyLogicalDevice(this);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	DestroyDebugMessenger();
	vkDestroyInstance(m_Instance, nullptr);
}

bool CVulkanRenderer::RendererBackend_Update()
{
	// Handle command buffer recording
	// TODO: Is this really the most efficient way to handle this?
	// Maybe caching off a reference to the command buffers (or just moving them to the main backend)
	// would be faster than doing this getter every single update.
	VkCommandBuffer& commandBuffer = m_Device.GetGraphicsCommandBuffer(m_CurrentImageIndex);
	
	if (!m_Device.BeginCommandBuffer(commandBuffer))
		return false;

	if (!m_Device.EndCommandBuffer(commandBuffer))
		return false;

	// We can't submit command buffers until we have sync objects in place to keep everything orderly.
	/*
	// Submit the command buffer for execution.
	if (!m_Device.SubmitCommandBuffer(commandBuffer))
		return false;*/


	m_CurrentFrame++;
	m_CurrentImageIndex = m_CurrentFrame % MAX_FRAMES_IN_FLIGHT;
	return true;
}

bool CVulkanRenderer::CreateVulkanInstance()
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

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.GetCount());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.Data();

		LOG_INFO("Enabled Vulkan Validation Layers: ");
		for (int i = 0; i < m_ValidationLayers.GetCount(); i++)
		{
			LOG_VERBOSE(m_ValidationLayers[i]);
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

	createInfo.enabledExtensionCount = extensionNames.GetCount();
	createInfo.ppEnabledExtensionNames = extensionNames.Data();

#if _DEBUG
	LOG_INFO("Enabled Vulkan Extensions: ");
	for (int i = 0; i < extensionNames.GetCount(); i++)
	{
		LOG_VERBOSE(extensionNames[i]);
	}
#endif

	// TODO: Custom allocator
	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		AssertMsg("Failed to create Vulkan Instance!");
		return false;
	}

	LOG_INFO("Vulkan Instance Created");
	return true;
}

bool CVulkanRenderer::CreateDebugMessenger()
{
#ifndef _DEBUG
	// No Vulkan Debug Callbacks in release builds!
	return true;
#endif

	VkDebugUtilsMessengerCreateInfoEXT createInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallbackFunc;

	PFN_vkCreateDebugUtilsMessengerEXT createDebuggerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (createDebuggerFunc != nullptr)
	{
		// TODO: Custom Allocator
		VkResult result = createDebuggerFunc(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		
		if (result != VK_SUCCESS)
		{
			AssertMsg("Failed to create Vulkan Debug Messenger!");
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
		LOG_FATAL("Failed to Craete Debug Messenger (VK_ERROR_EXTENSION_NOT_PRESENT)");
		return false;
	}

}

void CVulkanRenderer::DestroyDebugMessenger() const
{
#ifndef _DEBUG
	// No Vulkan Debug Callbacks in release builds!
	return;
#endif

	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebuggerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

	if (destroyDebuggerFunc != nullptr)
	{
		// TODO: Custom Allocator
		destroyDebuggerFunc(m_Instance, m_DebugMessenger, nullptr);
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

bool CVulkanRenderer::RequestValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	CVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());

	for (int i = 0; i < m_ValidationLayers.GetCount(); i++)
	{
		bool layerFound = false;
		for (int j = 0; i < availableLayers.GetCount(); j++)
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

