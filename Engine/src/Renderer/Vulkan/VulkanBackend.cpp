#include "VulkanBackend.h"

#include "Platform/VulkanPlatform.h"

#include "Core/Logging.h"

#include <String_Utils.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

bool CVulkanRenderer::RendererBackend_Initialize()
{
	// Cache off window dimensions here for later 
	// (it is VERY unlikely these will change again until the renderer is fully initialized)
	Platform_GetWindowSize(&m_FramebufferWidth, &m_FramebufferHeight);

	// Instance
	if (!CreateVulkanInstance())
		return false;

	// Debug Messenger
	if (!CreateDebugMessenger())
		return false;

	// Surface
	if (!Platform_CreateVulkanSurface(this))
		return false;

	// Device
	if (!VulkanDevice::CreateDevice(this, &m_Device))
		return false;

	// Swapchain
	if (!VulkanSwapchain::CreateSwapchain(this, &m_Swapchain))
		return false;

	// Main Renderpass
	if (!VulkanRenderpass::CreateRenderpass(this, &m_MainRenderpass))
		return false;

	// Command Pool
	if (!VulkanCommandPool::CreateCommandPool(this, &m_GraphicsCommandPool))
		return false;

	// Command Buffers
	// one per frame
	m_CommandBuffers.Resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < m_CommandBuffers.size(); i++)
	{
		if (!VulkanCommandBuffer::CreateCommandBuffer(this, true, &m_CommandBuffers[i]))
			return false;
	}

	// Framebuffers
	// one per frame
	m_Framebuffers.Resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < m_Framebuffers.size(); i++)
	{
		if (!VulkanFramebuffer::CreateFramebuffer(this, &m_Framebuffers[i]))
			return false;
	}

	// Fences
	// one per frame
	m_FrameInFlightFences.Resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < m_FrameInFlightFences.size(); i++)
	{
		if (!VulkanFence::CreateFence(this, true, &m_FrameInFlightFences[i]))
			return false;
	}

	// Semaphores
	// one per frame
	m_ImageAvailableSemaphores.Resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.Resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (!VulkanSemaphore::CreateVulkanSemaphore(this, &m_ImageAvailableSemaphores[i]))
			return false;

		if (!VulkanSemaphore::CreateVulkanSemaphore(this, &m_RenderFinishedSemaphores[i]))
			return false;
	}

	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	vkDeviceWaitIdle(m_Device.m_LogicalDevice);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_RenderFinishedSemaphores[i]);
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_ImageAvailableSemaphores[i]);
	}

	for (int i = 0; i < m_FrameInFlightFences.size(); i++)
		VulkanFence::DestroyFence(this, &m_FrameInFlightFences[i]);

	for (int i = 0; i < m_Framebuffers.size(); i++)
		VulkanFramebuffer::DestroyFramebuffer(this, &m_Framebuffers[i]);

	for (int i = 0; i < m_CommandBuffers.size(); i++)
		VulkanCommandBuffer::DestroyCommandBuffer(this, &m_CommandBuffers[i]);

	VulkanCommandPool::DestroyCommandPool(this, &m_GraphicsCommandPool);

	VulkanRenderpass::DestroyRenderpass(this, &m_MainRenderpass);

	VulkanSwapchain::DestroySwapchain(this, &m_Swapchain);

	VulkanDevice::DestroyDevice(this, &m_Device);
	
	vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
	DestroyDebugMessenger();

	delete m_ValidationLayers;

	vkDestroyInstance(m_Instance, m_Allocator);
}

bool CVulkanRenderer::RendererBackend_Update()
{
	// Wait for the previous frame to finish it's work
	vkWaitForFences(m_Device.m_LogicalDevice, 1, &m_FrameInFlightFences[m_CurrentFrame].m_Handle, VK_TRUE, UINT64_MAX);

	unsigned int imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device.m_LogicalDevice, m_Swapchain.m_SwapchainHandle, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame].m_Handle, VK_NULL_HANDLE, &imageIndex);

	// TODO: Swapchain recreation

	vkResetFences(m_Device.m_LogicalDevice, 1, &m_FrameInFlightFences[m_CurrentFrame].m_Handle);

	VulkanCommandBuffer& commandBuffer = m_CommandBuffers[m_CurrentFrame];

	if (!commandBuffer.m_Handle)
	{
		LOG_ERROR("CVulkanRednerer::Update() -> Failed to obtain frame command buffer.");
		return false;
	}

	commandBuffer.Reset();
	commandBuffer.Begin();

		if (!m_MainRenderpass.Begin(this, &commandBuffer, imageIndex))
		{
			LOG_ERROR("Failed to begin Vulkan Renderpass");
			return false;
		}

		// TODO: Handle all draw calls here

		m_MainRenderpass.End(this, &commandBuffer);

	commandBuffer.End();

	// Submit the command buffer for execution
	// TODO: Move this to the swapchain.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame].m_Handle;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer.m_Handle;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame].m_Handle;

	if (!VK_CHECK(vkQueueSubmit(m_Device.m_GraphicsQueue, 1, &submitInfo, m_FrameInFlightFences[m_CurrentFrame].m_Handle)))
	{
		LOG_ERROR("Failed to submit command buffer");
		return false;
	}
	commandBuffer.m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_PENDING;

	// Present the framebuffer
	// TODO: Move this to the swapchain.
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame].m_Handle;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain.m_SwapchainHandle;
	presentInfo.pImageIndices = &imageIndex;

	VkResult presentResult = vkQueuePresentKHR(m_Device.m_PresentQueue, &presentInfo);
	commandBuffer.m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INVALID;
	// TODO: Swapchain recreation

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
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

	instanceInfo.enabledExtensionCount = requiredExtensions.size();
	instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();

	// Validation Layers
	if (EnableValidationLayers())
	{
		m_ValidationLayers = new CVector<const char*>;
		m_ValidationLayers->PushBack("VK_LAYER_KHRONOS_validation");

		if (!QueryValidationLayerSupport(*m_ValidationLayers))
		{
			LOG_ERROR("Required Vulkan validation layers were not found - Application cannot continue.");
			return false;
		}

		instanceInfo.enabledLayerCount = m_ValidationLayers->size();
		instanceInfo.ppEnabledLayerNames = m_ValidationLayers->data();
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
		for (int i = 0; i < requiredExtensions.size(); i++)
		{
			LOG_DEBUG("%s", requiredExtensions[i]);
		}

		LOG_INFO("Enabled Validation Layers:");
		
		for (int i = 0; i < m_ValidationLayers->size(); i++)
		{
			LOG_DEBUG("%s", m_ValidationLayers->data()[i]);
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
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

	for (unsigned int i = 0; i < requiredExtensions.size(); i++)
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
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	for (unsigned int i = 0; i < requiredValidationLayers.size(); i++)
	{
		bool bFound = false;
		for (unsigned int j = 0; j < layerCount; j++)
		{
			const char* currentValidationLayer = requiredValidationLayers[i];
			if (StringCompare(currentValidationLayer, availableLayers[j].layerName))
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
		return true;
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

