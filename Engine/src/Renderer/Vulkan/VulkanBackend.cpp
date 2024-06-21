//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#include "VulkanBackend.h"
#include "Platform/VulkanPlatform.h"
#include "Systems/IMemory.h"
#include "Systems/IEvent.h"
#include "Systems/IFileSystem.h"

#include "Core/Assertions.h"

#include <String_Utils.h>
#include <stdint.h>


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

static bool WindowResizedCallback(EventData_t eventData);


IRendererBackend* GetVulkanBackend()
{
	if (!VulkanRenderingContext)
		VulkanRenderingContext = new CVulkanRenderer;
	
	return VulkanRenderingContext;
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

	if (!VulkanRenderpass::CreateRenderpass(this, &m_MainRenderpass))
		return false;

	if (!VulkanShader::CreateVulkanShader(this, &m_StandardShader))
		return false;

	if (!m_Device.CreateCommandBuffers(this))
		return false;

	if (!m_Swapchain.CreateFramebuffers(this))
		return false;

	if (!CreateSyncObjects())
		return false;

	// Register for window-resize event callback.
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, WindowResizedCallback);

	LOG_INFO("Vulkan Rendering Backend Initialized");
	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	LOG_INFO("Vulkan Rendering Backend Shutting Down");
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());

	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, WindowResizedCallback);

	// Shutdown renderer components in reverse order.
	// TODO: Custom allocator.

	m_Swapchain.CleanupSwapchain(this);
	DestroySyncObjects();
	m_Device.DestroyCommandBuffers(this);
	m_StandardShader.DestroyVulkanShader(this);
	m_MainRenderpass.DestroyRenderpass(this, &m_MainRenderpass);
	m_Device.DestroyLogicalDevice(this);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	DestroyDebugMessenger();
	vkDestroyInstance(m_Instance, nullptr);
}

bool CVulkanRenderer::RendererBackend_Update()
{
	// Wait for the previous frame to finish it's work
	vkWaitForFences(m_Device.GetLogicalDevice(), 1, &m_InFlightFrameFences[m_CurrentImageIndex], VK_TRUE, UINT64_MAX);

	// Acquire the next image from the swapchain
	unsigned int currImageIndex = 0;
	VkResult nextImageResult = vkAcquireNextImageKHR(m_Device.GetLogicalDevice(), m_Swapchain.GetHandle(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentImageIndex], 0, &currImageIndex);
	if (nextImageResult != VK_SUCCESS)
	{
		if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			//LOG_DEBUG("Swapchain out of date - Recreating.");
			m_Swapchain.RecreateSwapchain(this);
			//m_FramebufferResized = true;
			return true;
		}
		else if (nextImageResult != VK_SUCCESS && nextImageResult != VK_SUBOPTIMAL_KHR)
		{
			LOG_ERROR("Failed to get next swapchain image");
			return false;
		}
	}

	vkResetFences(m_Device.GetLogicalDevice(), 1, &m_InFlightFrameFences[m_CurrentImageIndex]);

	// Handle command buffer recording
	// TODO: Is this really the most efficient way to handle this?
	// Maybe caching off a reference to the command buffers (or just moving them to the main backend)
	// would be faster than doing this getter every single update.
	VkCommandBuffer& commandBuffer = m_Device.GetGraphicsCommandBuffer(m_CurrentImageIndex);
	vkResetCommandBuffer(commandBuffer, 0);
	if (!m_Device.BeginCommandBuffer(commandBuffer))
		return false;

	m_MainRenderpass.BeginRenderpass(this, &commandBuffer);

		// Handle all draw/rendering calls here
		m_StandardShader.BindShader(this, &commandBuffer);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	m_MainRenderpass.EndRenderpass(this, &commandBuffer);

	if (!m_Device.EndCommandBuffer(commandBuffer))
		return false;
	
	// Submit the command buffer for execution.
	if (!m_Device.SubmitCommandBuffer(this, commandBuffer, m_CurrentImageIndex))
		return false;

	if (!m_Swapchain.FramebufferPresent(this, currImageIndex))
	{
		LOG_ERROR("Failed to present swapchain image!");
		return false;
	}

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

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_NumValidationLayers);
		createInfo.ppEnabledLayerNames = m_ValidationLayers;

		LOG_INFO("Enabled Vulkan Validation Layers: ");
		for (int i = 0; i < m_NumValidationLayers; i++)
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

	for (int i = 0; i < m_NumValidationLayers; i++)
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

bool CVulkanRenderer::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

	// Create our fence in the 'signaled' state so the first frame doesn't have to wait on it.
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// TODO: Custom allocator
		if (vkCreateSemaphore(m_Device.GetLogicalDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device.GetLogicalDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_Device.GetLogicalDevice(), &fenceInfo, nullptr, &m_InFlightFrameFences[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create syncronization primitives for Vulkan!");
			return false;
		}
	}

	LOG_INFO("Vulkan Sync Primitives Created");
	return true;
}

void CVulkanRenderer::DestroySyncObjects()
{
	// TODO: Custom allocator
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_Device.GetLogicalDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device.GetLogicalDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_Device.GetLogicalDevice(), m_InFlightFrameFences[i], nullptr);
	}
}

bool WindowResizedCallback(EventData_t eventData)
{
	// Tell the swapchain framebuffer to resize.
	VulkanRenderingContext->SetFramebufferResized(true);
	return true;
}

