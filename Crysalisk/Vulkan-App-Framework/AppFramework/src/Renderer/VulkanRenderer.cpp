//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2025
// 
// Purpose: The Vulkan Renderer.
//=============================================================================
#include "pch.h"
#include "VulkanRenderer.h"

#include "VulkanPlatform.h"
#include "VulkanUtils.h"

#include "Math/MathLib.h"

#include <vulkan/vulkan.h>

// TODO: Use our own filesystem.
#include <stdio.h>

//-------------------------------------------------------------------------
// The Vulkan renderer.
//-------------------------------------------------------------------------
class CVulkanRenderer : public IVulkanRenderer
{
public:
	// Initialization, shutdown.
	virtual bool	Initialize(void* pHWND, uint32_t inWidth, uint32_t inHeight) override;
	virtual void	Shutdown() override;
	
	virtual void	BeginFrame() override;
	virtual void	EndFrame() override;

	virtual void	RenderGeometry(GeometryInstanceData_t& pGeometry, uint8_t iFlags) override;

	virtual void	GetRenderSurfaceDimensions(uint32_t& pOutWidth, uint32_t& pOutHeight) override { pOutWidth = m_Width; pOutHeight = m_Height; }

	virtual GeometryInstanceData_t	AcquireGeometryResources(uint32_t vertCount, Vector2D* pVertices) override;

private:
	bool			CreateVulkanDevice();
	bool			CreateSwapchain();
	void			RecreateSwapchain();

	struct VulkanBuffer_t;
	bool			CreateBuffer(size_t iBufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties, VulkanBuffer_t& pOutBuffer);

	// CONTROLTODO: Just a test shader for now.
	// These should be loaded as needed, by name.
	bool			CreateShaders();
	bool			CreateShaderModule(const char* pszPath, std::vector<VkShaderModule>& pOutModules);

	void			TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	uint32_t					m_Width{ 0 }, m_Height{ 0 };

	// Instance
	VkInstance					m_Instance{ VK_NULL_HANDLE };

	// Debug messenger
#ifdef _DEBUG
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };
#endif

	// Window surface
	VkSurfaceKHR				m_WindowSurface{ VK_NULL_HANDLE };

	// Device
	struct VulkanDevice_t
	{
		// Device handles.
		VkPhysicalDevice	hPhysicalDevice{ VK_NULL_HANDLE };
		VkDevice			hLogicalDevice{ VK_NULL_HANDLE };

		// Device details.
		VkPhysicalDeviceFeatures			deviceFeatures{};
		VkPhysicalDeviceProperties			deviceProperties{};
		VkPhysicalDeviceMemoryProperties	memoryProperties{};

		// Queue family indices.
		int					GraphicsQueueFamilyIndex{ -1 };
		int					PresentQueueFamilyIndex{ -1 };

		// Device queues.
		VkQueue				hGraphicsQueue{ VK_NULL_HANDLE };
		VkQueue				hPresentQueue{ VK_NULL_HANDLE };
	} m_VulkanDevice;

	// Swapchain
	struct VulkanSwapchain_t
	{
		VkSwapchainKHR				hSwapchain{ VK_NULL_HANDLE };
		std::vector<VkImage>		swapchainImages{};
		std::vector<VkImageView>	swapchainImageViews{};

		uint32_t					iImageCount{ 0 };
		VkSurfaceCapabilitiesKHR	surfaceCapabilities{};
		VkFormat					swapchainImageFormat{ VK_FORMAT_UNDEFINED };
		VkExtent2D					swapchainExtent{};
	} m_Swapchain;

	// Buffers
	struct VulkanBuffer_t
	{
		VkBuffer		hBuffer{ VK_NULL_HANDLE };
		VkDeviceMemory	bufferMemory{ VK_NULL_HANDLE };
		uint64_t		iCurrentOffset{ 0 }; // TODO: Freelist!
	};

	// Shaders
	struct VulkanShader_t
	{
		VkPipeline					pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout			pipelineLayout{ VK_NULL_HANDLE };
	};

	// Command pools/buffers
	// One per frame.
	std::vector<VkCommandPool>		m_GraphicsCommandPools{};
	std::vector<VkCommandBuffer>	m_GraphicsCommandBuffers{};

	// Synchronization primitives
	// One per frame.
	std::vector<VkSemaphore>		m_ImageAvailableSemaphores{};
	std::vector<VkSemaphore>		m_RenderCompleteSemaphores{};
	std::vector<VkFence>			m_FrameCompleteFences{};

	// Geometry buffers.
	VulkanBuffer_t					m_GeometryVertexBuffer{};

	// Shaders
	VulkanShader_t					m_TestShader{};

	uint32_t						m_iCurrentImageIndex{ 0 };
	uint64_t						m_iFrameNumber{ 0 };
};

// Vulkan renderer context.
static CVulkanRenderer s_VulkanContext;
IVulkanRenderer* g_pRenderer = &s_VulkanContext;

// Forward Declarations
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
	std::printf("%s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

//-------------------------------------------------------------------------
// Purpose: Initialize the Vulkan renderer.
//-------------------------------------------------------------------------
bool CVulkanRenderer::Initialize(void* pHWND, uint32_t inWidth, uint32_t inHeight)
{
	m_Width = inWidth;
	m_Height = inHeight;

	// Setup the Vulkan Instance.
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Vulkan App Framework"; // TODO: Pass this in from a config.
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "Vulkan App Framework"; // TODO: Framework name.
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);	// NOTE: We need to be AT LEAST 1.2.197 for dynamic rendering.

	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &appInfo;

	// TODO: Check for the presence of validation layers and extensions!
	// Validation layers.
	std::vector<const char*> szValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> szExtensionNames = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VulkanPlatform::GetPlatformSurfaceExtension() };

#ifdef _DEBUG
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = szValidationLayers.data();

	instanceInfo.enabledExtensionCount = (uint32_t)szExtensionNames.size();
	instanceInfo.ppEnabledExtensionNames = szExtensionNames.data();
#else
	instanceInfo.enabledLayerCount = 0;
	instanceInfo.ppEnabledLayerNames = nullptr;

	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = nullptr;
#endif

	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));

	// Create the debug messenger.
#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debuggerCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debuggerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debuggerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debuggerCreateInfo.pfnUserCallback = VulkanDebugCallback;
	
	PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (!pfnCreateDebugger)
	{
		// TODO: Logging
		return false;
	}
	else
	{
		if (pfnCreateDebugger(m_Instance, &debuggerCreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
		{
			// TODO: Logging
			return false;
		}
	}
#endif

	// Create the window surface.
	m_WindowSurface = VulkanPlatform::CreateWindowSurface(m_Instance, pHWND);
	if (m_WindowSurface == VK_NULL_HANDLE)
	{
		// TODO: Logging.
		return false;
	}

	// Device creation.
	if (!CreateVulkanDevice())
	{
		// TODO: Logging.
		return false;
	}

	// Swapchain creation.
	if (!CreateSwapchain())
	{
		// TODO: Logging.
		return false;
	}

	// Graphics command pool/buffers.
	VkCommandPoolCreateInfo	graphicsCommandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	graphicsCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsCommandPoolInfo.queueFamilyIndex = m_VulkanDevice.GraphicsQueueFamilyIndex;

	m_GraphicsCommandPools.resize(kMaxFramesInFlight);
	m_GraphicsCommandBuffers.resize(kMaxFramesInFlight);

	for (int iFrame = 0; iFrame < kMaxFramesInFlight; iFrame++)
	{
		VK_CHECK(vkCreateCommandPool(m_VulkanDevice.hLogicalDevice, &graphicsCommandPoolInfo, nullptr, &m_GraphicsCommandPools[iFrame]));

		// Also allocate the main framebuffer while we're at it.
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = m_GraphicsCommandPools[iFrame];
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VK_CHECK(vkAllocateCommandBuffers(m_VulkanDevice.hLogicalDevice, &allocInfo, &m_GraphicsCommandBuffers[iFrame]));
	}

	// Sync primitives.
	VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	m_ImageAvailableSemaphores.resize(kMaxFramesInFlight);
	m_RenderCompleteSemaphores.resize(kMaxFramesInFlight);
	m_FrameCompleteFences.resize(kMaxFramesInFlight);
	for (int iFrame = 0; iFrame < kMaxFramesInFlight; iFrame++)
	{
		VK_CHECK(vkCreateSemaphore(m_VulkanDevice.hLogicalDevice, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[iFrame]));
		VK_CHECK(vkCreateSemaphore(m_VulkanDevice.hLogicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderCompleteSemaphores[iFrame]));
		VK_CHECK(vkCreateFence(m_VulkanDevice.hLogicalDevice, &fenceCreateInfo, nullptr, &m_FrameCompleteFences[iFrame]));
	}

	// Create geometry buffers.
	if (!CreateBuffer(sizeof(Vector2D) * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, m_GeometryVertexBuffer))
	{
		// TODO: Logging
		return false;
	}

	if (!CreateShaders())
	{
		// TODO: Logging
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the Vulkan renderer.
//-------------------------------------------------------------------------
void CVulkanRenderer::Shutdown()
{
	// Make sure no resources are in use!
	vkDeviceWaitIdle(m_VulkanDevice.hLogicalDevice);

	// TODO: temp shaders
	if (m_TestShader.pipeline)
	{
		vkDestroyPipeline(m_VulkanDevice.hLogicalDevice, m_TestShader.pipeline, nullptr);
		m_TestShader.pipeline = VK_NULL_HANDLE;
	}
	if (m_TestShader.pipelineLayout)
	{
		vkDestroyPipelineLayout(m_VulkanDevice.hLogicalDevice, m_TestShader.pipelineLayout, nullptr);
		m_TestShader.pipelineLayout = VK_NULL_HANDLE;
	}

	// Geometry buffers.
	m_GeometryVertexBuffer.iCurrentOffset = 0;
	if (m_GeometryVertexBuffer.bufferMemory)
	{
		vkFreeMemory(m_VulkanDevice.hLogicalDevice, m_GeometryVertexBuffer.bufferMemory, nullptr);
		m_GeometryVertexBuffer.bufferMemory = VK_NULL_HANDLE;
	}
	if (m_GeometryVertexBuffer.hBuffer)
	{
		vkDestroyBuffer(m_VulkanDevice.hLogicalDevice, m_GeometryVertexBuffer.hBuffer, nullptr);
		m_GeometryVertexBuffer.hBuffer = VK_NULL_HANDLE;
	}

	// Sync primitives.
	for (int iFrame = 0; iFrame < kMaxFramesInFlight; iFrame++)
	{
		vkDestroySemaphore(m_VulkanDevice.hLogicalDevice, m_ImageAvailableSemaphores[iFrame], nullptr);
		m_ImageAvailableSemaphores[iFrame] = VK_NULL_HANDLE;
		vkDestroySemaphore(m_VulkanDevice.hLogicalDevice, m_RenderCompleteSemaphores[iFrame], nullptr);
		m_RenderCompleteSemaphores[iFrame] = VK_NULL_HANDLE;
		vkDestroyFence(m_VulkanDevice.hLogicalDevice, m_FrameCompleteFences[iFrame], nullptr);
		m_FrameCompleteFences[iFrame] = VK_NULL_HANDLE;
	}

	// Command pools
	for (int iFrame = 0; iFrame < kMaxFramesInFlight; iFrame++)
	{
		vkDestroyCommandPool(m_VulkanDevice.hLogicalDevice, m_GraphicsCommandPools[iFrame], nullptr);
		m_GraphicsCommandPools[iFrame] = VK_NULL_HANDLE;
	}

	// Swapchain
	for (uint32_t idx = 0; idx < m_Swapchain.iImageCount; idx++)
	{
		vkDestroyImageView(m_VulkanDevice.hLogicalDevice, m_Swapchain.swapchainImageViews[idx], nullptr);
		m_Swapchain.swapchainImageViews[idx] = VK_NULL_HANDLE;
	}

	if (m_Swapchain.hSwapchain)
	{
		vkDestroySwapchainKHR(m_VulkanDevice.hLogicalDevice, m_Swapchain.hSwapchain, nullptr);
		m_Swapchain.hSwapchain = VK_NULL_HANDLE;
	}

	// Device
	if (m_VulkanDevice.hLogicalDevice)
	{
		vkDestroyDevice(m_VulkanDevice.hLogicalDevice, nullptr);
		m_VulkanDevice.hLogicalDevice = VK_NULL_HANDLE;
	}

	// Window surface
	if (m_WindowSurface)
	{
		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		m_WindowSurface = VK_NULL_HANDLE;
	}

#if _DEBUG
	// Debug messenger.
	if (m_DebugMessenger)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (pfnDestroyDebugger)
		{
			pfnDestroyDebugger(m_Instance, m_DebugMessenger, nullptr);
		}
		m_DebugMessenger = VK_NULL_HANDLE;
	}
#endif

	// Instance.
	if (m_Instance)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
GeometryInstanceData_t CVulkanRenderer::AcquireGeometryResources(uint32_t vertCount, Vector2D* pVertices)
{
	// TODO: Make sure inputs are valid!

	GeometryInstanceData_t newGeometryData{};

	newGeometryData.vertexCount = vertCount;
	newGeometryData.vertexDataOffset = m_GeometryVertexBuffer.iCurrentOffset;
	newGeometryData.vertexDataSize = vertCount * sizeof(pVertices);

	// Upload the data to the GPU.
	void* pData{ nullptr };
	vkMapMemory(m_VulkanDevice.hLogicalDevice, m_GeometryVertexBuffer.bufferMemory, newGeometryData.vertexDataOffset, newGeometryData.vertexDataSize, 0, &pData);
	memcpy(pData, pVertices, (size_t)newGeometryData.vertexDataSize);
	vkUnmapMemory(m_VulkanDevice.hLogicalDevice, m_GeometryVertexBuffer.bufferMemory);

	m_GeometryVertexBuffer.iCurrentOffset += newGeometryData.vertexDataSize;

	return newGeometryData;
}

//-------------------------------------------------------------------------
// Purpose: Render loop.
//-------------------------------------------------------------------------
void CVulkanRenderer::BeginFrame()
{
	// Wait for the previous frame to finish rendering.
	VK_CHECK(vkWaitForFences(m_VulkanDevice.hLogicalDevice, 1, &m_FrameCompleteFences[m_iCurrentImageIndex], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(m_VulkanDevice.hLogicalDevice, 1, &m_FrameCompleteFences[m_iCurrentImageIndex]));

	// TODO: Check if the swapchain needs to be recreated.
	// Get the next image from the swapchain.
	VkResult imageAcquireResult = vkAcquireNextImageKHR(m_VulkanDevice.hLogicalDevice, m_Swapchain.hSwapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_iCurrentImageIndex], nullptr, &m_iCurrentImageIndex);

	if (imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// We need to recreate our swapchain.
		vkDeviceWaitIdle(m_VulkanDevice.hLogicalDevice);

		RecreateSwapchain();

		return;
	}

	// Begin rendering.
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_iCurrentImageIndex];
	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

	VkCommandBufferBeginInfo cmdBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

	// Transition swapchain image to layout for being rendered to.
	TransitionImageLayout(commandBuffer, m_Swapchain.swapchainImages[m_iCurrentImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// TODO: Render commands here!
	// Test screen clear:
	VkClearColorValue clearValue{};
	/*float flBlue = fabsf(sinf(m_iFrameNumber / 560.0f));
	clearValue = { { 0.0f, 0.0f, flBlue, 1.0f } };*/
	clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdClearColorImage(commandBuffer, m_Swapchain.swapchainImages[m_iCurrentImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &subresourceRange);

	VkRenderingAttachmentInfo colorAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachmentInfo.imageView = m_Swapchain.swapchainImageViews[m_iCurrentImageIndex];	// Just rendering to the backbuffer for now.
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderInfo.renderArea.extent = m_Swapchain.swapchainExtent;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachmentInfo;
	renderInfo.layerCount = 1;
	vkCmdBeginRendering(commandBuffer, &renderInfo);

	// Set dynamic state.
	VkViewport viewport{};
	viewport.x = viewport.y = 0;
	viewport.width = (float)m_Swapchain.swapchainExtent.width;
	viewport.height = (float)m_Swapchain.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain.swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
void CVulkanRenderer::EndFrame()
{
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_iCurrentImageIndex];
	
	vkCmdEndRendering(commandBuffer);
	
	// Transition swapchain image to layout for presenting.
	TransitionImageLayout(commandBuffer, m_Swapchain.swapchainImages[m_iCurrentImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	// Submit the command buffer to the queue.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_iCurrentImageIndex];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderCompleteSemaphores[m_iCurrentImageIndex];
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(m_VulkanDevice.hGraphicsQueue, 1, &submitInfo, m_FrameCompleteFences[m_iCurrentImageIndex]));

	// Present.
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain.hSwapchain;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderCompleteSemaphores[m_iCurrentImageIndex];
	presentInfo.pImageIndices = &m_iCurrentImageIndex;

	VkResult imagePresentResult = vkQueuePresentKHR(m_VulkanDevice.hPresentQueue, &presentInfo);

	if (imagePresentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// We need to recreate our swapchain.
		vkDeviceWaitIdle(m_VulkanDevice.hLogicalDevice);

		RecreateSwapchain();

		return;
	}

	m_iCurrentImageIndex = (m_iCurrentImageIndex + 1) % kMaxFramesInFlight;
	m_iFrameNumber++;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
void CVulkanRenderer::RenderGeometry(GeometryInstanceData_t& pGeometry, uint8_t iFlags)
{
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_iCurrentImageIndex];

	int32_t iHovered = 0;
	if ((iFlags & GEOMETRY_RENDER_FLAG_CONTROL_HOVERED_OVER) != 0)
	{
		iHovered = 1;
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TestShader.pipeline);
	VkDeviceSize offset = pGeometry.vertexDataOffset;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_GeometryVertexBuffer.hBuffer, &offset);

	vkCmdPushConstants(commandBuffer, m_TestShader.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int32_t), &iHovered);

	// Draw!
	vkCmdDraw(commandBuffer, (uint32_t)pGeometry.vertexCount, 1, 0, 0);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateVulkanDevice()
{
	// Find a physical device.
	unsigned int iPhysicalDeviceCount{ 0 };
	vkEnumeratePhysicalDevices(m_Instance, &iPhysicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> availableDevices(iPhysicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &iPhysicalDeviceCount, availableDevices.data());

	// Use any available GPU, but prefer discrete hardware.
	VkPhysicalDevice selectedDevice{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties deviceProperties{};
	for (uint32_t iDevice = 0; iDevice < iPhysicalDeviceCount; iDevice++)
	{
		vkGetPhysicalDeviceProperties(availableDevices[iDevice], &deviceProperties);

		if (deviceProperties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			// Discrete GPU found!
			selectedDevice = availableDevices[iDevice];
			break;
		}

		if (selectedDevice == VK_NULL_HANDLE)
		{
			// Only assign this if the first device.
			selectedDevice = availableDevices[iDevice];
			break;
		}
	}

	if (selectedDevice == VK_NULL_HANDLE)
	{
		// TODO: Logging.
		return false;
	}
	else
	{
		// Cache off the selected device.
		m_VulkanDevice.hPhysicalDevice = selectedDevice;
	}

	// Query physical device queue information.
	unsigned int iDeviceQueueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &iDeviceQueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(iDeviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &iDeviceQueueFamilyCount, queueFamilyProperties.data());

	for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < iDeviceQueueFamilyCount; queueFamilyIndex++)
	{
		if ((queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (m_VulkanDevice.GraphicsQueueFamilyIndex < 1))
		{
			// Found a queue family index for graphics.
			m_VulkanDevice.GraphicsQueueFamilyIndex = queueFamilyIndex;
		}

		// Check for present support as well
		VkBool32 bPresentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_VulkanDevice.hPhysicalDevice, queueFamilyIndex, m_WindowSurface, &bPresentSupport);

		if (bPresentSupport && (m_VulkanDevice.PresentQueueFamilyIndex < 1))
		{
			// Found a queue family index for presenting.
			m_VulkanDevice.PresentQueueFamilyIndex = queueFamilyIndex;
		}
	}

	// Graphics and presenting are required!
	if (m_VulkanDevice.GraphicsQueueFamilyIndex < 0 && m_VulkanDevice.PresentQueueFamilyIndex < 0)
	{
		// TODO: Logging.
		return false;
	}

	// Now we setup the logical device.
	std::vector<const char*> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

	const float flQueuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfos[2];	// [0] = Graphics queue, [1] = Present queue
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = m_VulkanDevice.GraphicsQueueFamilyIndex;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &flQueuePriority;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].pNext = nullptr;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = m_VulkanDevice.PresentQueueFamilyIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = &flQueuePriority;
	queueCreateInfos[1].flags = 0;
	queueCreateInfos[1].pNext = nullptr;

	// Dynamic rendering setup.
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	dynamicRendering.dynamicRendering = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 2;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = &dynamicRendering;	// Chaining on the dynamic rendering extension data.
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

	VK_CHECK(vkCreateDevice(m_VulkanDevice.hPhysicalDevice, &deviceCreateInfo, nullptr, &m_VulkanDevice.hLogicalDevice));

	// Obtain queue handles.
	vkGetDeviceQueue(m_VulkanDevice.hLogicalDevice, m_VulkanDevice.GraphicsQueueFamilyIndex, 0, &m_VulkanDevice.hGraphicsQueue);
	vkGetDeviceQueue(m_VulkanDevice.hLogicalDevice, m_VulkanDevice.PresentQueueFamilyIndex, 0, &m_VulkanDevice.hPresentQueue);

	// Cache off device details.
	vkGetPhysicalDeviceFeatures(m_VulkanDevice.hPhysicalDevice, &m_VulkanDevice.deviceFeatures);
	vkGetPhysicalDeviceProperties(m_VulkanDevice.hPhysicalDevice, &m_VulkanDevice.deviceProperties);
	vkGetPhysicalDeviceMemoryProperties(m_VulkanDevice.hPhysicalDevice, &m_VulkanDevice.memoryProperties);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateSwapchain()
{
	VulkanSwapchain_t& pOutSwapchain = m_Swapchain;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VulkanDevice.hPhysicalDevice, m_WindowSurface, &m_Swapchain.surfaceCapabilities);

	uint32_t iSurfaceFormatCount{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanDevice.hPhysicalDevice, m_WindowSurface, &iSurfaceFormatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(iSurfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanDevice.hPhysicalDevice, m_WindowSurface, &iSurfaceFormatCount, surfaceFormats.data());

	if (iSurfaceFormatCount < 1)
	{
		// TODO: Logging.
		return false;
	}

	uint32_t iPresentModeCount{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanDevice.hPhysicalDevice, m_WindowSurface, &iPresentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(iPresentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanDevice.hPhysicalDevice, m_WindowSurface, &iPresentModeCount, presentModes.data());

	if (iPresentModeCount < 1)
	{
		// TODO: Logging.
		return false;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = m_WindowSurface;

	if (pOutSwapchain.surfaceCapabilities.minImageCount < kMaxFramesInFlight && pOutSwapchain.surfaceCapabilities.maxImageCount > kMaxFramesInFlight)
	{
		swapchainCreateInfo.minImageCount = kMaxFramesInFlight;
	}
	else
	{
		// Just use the minimum count supported.
		swapchainCreateInfo.minImageCount = pOutSwapchain.surfaceCapabilities.minImageCount;
	}

	// We're going to render to the entire window surface.
	swapchainCreateInfo.imageExtent = m_Swapchain.surfaceCapabilities.currentExtent;

	// Our preferred format/color space is: B8G8R8A8_SRGB and SRGB_NONLINEAR.
	for (int idx = 0; idx < surfaceFormats.size(); idx++)
	{
		if (surfaceFormats[idx].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[idx].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			// Found 'em.
			swapchainCreateInfo.imageFormat = surfaceFormats[idx].format;
			swapchainCreateInfo.imageColorSpace = surfaceFormats[idx].colorSpace;
			break;
		}
	}

	if (swapchainCreateInfo.imageFormat == VK_FORMAT_UNDEFINED)
	{
		// If we can't find our preferred format, just use the first available.
		swapchainCreateInfo.imageFormat = surfaceFormats[0].format;
		swapchainCreateInfo.imageColorSpace = surfaceFormats[0].colorSpace;
	}

	// Our preferred present mode is MAILBOX_KHR.
	for (int idx = 0; idx < presentModes.size(); idx++)
	{
		if (presentModes[idx] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			// Found it.
			swapchainCreateInfo.presentMode = presentModes[idx];
		}
	}

	if (swapchainCreateInfo.presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	{
		// Fall back on this present mode if needed, as it is the only one guaranteed to exist.
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}

	swapchainCreateInfo.imageArrayLayers = pOutSwapchain.surfaceCapabilities.maxImageArrayLayers;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;	// NOTE: We need to be marked as a transfer-dest for vkCmdClearColorImage.
	swapchainCreateInfo.preTransform = pOutSwapchain.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VK_CHECK(vkCreateSwapchainKHR(m_VulkanDevice.hLogicalDevice, &swapchainCreateInfo, nullptr, &pOutSwapchain.hSwapchain));

	// Cache off relevant formats.
	pOutSwapchain.swapchainImageFormat = swapchainCreateInfo.imageFormat;
	pOutSwapchain.swapchainExtent = swapchainCreateInfo.imageExtent;

	// Get the swapchain images.
	vkGetSwapchainImagesKHR(m_VulkanDevice.hLogicalDevice, pOutSwapchain.hSwapchain, &pOutSwapchain.iImageCount, nullptr);
	pOutSwapchain.swapchainImages.resize(pOutSwapchain.iImageCount);
	vkGetSwapchainImagesKHR(m_VulkanDevice.hLogicalDevice, pOutSwapchain.hSwapchain, &pOutSwapchain.iImageCount, pOutSwapchain.swapchainImages.data());

	// Create swapchain image views.
	pOutSwapchain.swapchainImageViews.resize(pOutSwapchain.iImageCount);
	for (uint32_t imageIdx = 0; imageIdx < pOutSwapchain.iImageCount; imageIdx++)
	{
		VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewCreateInfo.image = pOutSwapchain.swapchainImages[imageIdx];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = pOutSwapchain.swapchainImageFormat;
		viewCreateInfo.components.r = viewCreateInfo.components.g = viewCreateInfo.components.b = viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(m_VulkanDevice.hLogicalDevice, &viewCreateInfo, nullptr, &pOutSwapchain.swapchainImageViews[imageIdx]));
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
void CVulkanRenderer::RecreateSwapchain()
{
	// TODO: Don't recreate if we are shutting down.
	// TODO: Logging.
	std::printf("Recreating swapchain\n");
	

	for (uint32_t idx = 0; idx < m_Swapchain.iImageCount; idx++)
	{
		vkDestroyImageView(m_VulkanDevice.hLogicalDevice, m_Swapchain.swapchainImageViews[idx], nullptr);
		m_Swapchain.swapchainImageViews[idx] = VK_NULL_HANDLE;
	}

	if (m_Swapchain.hSwapchain)
	{
		vkDestroySwapchainKHR(m_VulkanDevice.hLogicalDevice, m_Swapchain.hSwapchain, nullptr);
		m_Swapchain.hSwapchain = VK_NULL_HANDLE;
	}

	CreateSwapchain();
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
static uint32_t FindMemoryType(VkPhysicalDeviceMemoryProperties& pMemProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t idx = 0; idx < pMemProperties.memoryTypeCount; idx++)
	{
		if (typeFilter & (1 << idx) && (pMemProperties.memoryTypes[idx].propertyFlags & properties) == properties)
		{
			return idx;
		}
	}

	// TODO: Logging.
	return 0;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateBuffer(size_t iBufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties, VulkanBuffer_t& pOutBuffer)
{
	VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = iBufferSize;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(m_VulkanDevice.hLogicalDevice, &bufferCreateInfo, nullptr, &pOutBuffer.hBuffer));

	VkMemoryRequirements bufferRequirements{};
	vkGetBufferMemoryRequirements(m_VulkanDevice.hLogicalDevice, pOutBuffer.hBuffer, &bufferRequirements);
	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = bufferRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(m_VulkanDevice.memoryProperties, bufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VK_CHECK(vkAllocateMemory(m_VulkanDevice.hLogicalDevice, &allocInfo, nullptr, &pOutBuffer.bufferMemory));

	vkBindBufferMemory(m_VulkanDevice.hLogicalDevice, pOutBuffer.hBuffer, pOutBuffer.bufferMemory, 0);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateShaderModule(const char* pszPath, std::vector<VkShaderModule>& pOutModules)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

	FILE* pFile{};
	if (fopen_s(&pFile, pszPath, "rb") != 0)
	{
		return false;
	}

	// Shader code size.
	std::vector<uint32_t*> shaderBytecode{};
	{
		fseek(pFile, 0, SEEK_END);
		size_t fileSize = ftell(pFile);
		rewind(pFile);
		shaderBytecode.reserve(fileSize);
		shaderModuleCreateInfo.codeSize = fileSize;
	}
	fread_s(shaderBytecode.data(), shaderModuleCreateInfo.codeSize, 1, shaderModuleCreateInfo.codeSize, pFile);
	shaderModuleCreateInfo.pCode = (uint32_t*)shaderBytecode.data();

	fclose(pFile);

	VkShaderModule newModule{ VK_NULL_HANDLE };
	VK_CHECK(vkCreateShaderModule(m_VulkanDevice.hLogicalDevice, &shaderModuleCreateInfo, nullptr, &newModule));

	pOutModules.push_back(newModule);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateShaders()
{
	// TODO: Make this configurable!
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Setup shader modules.
	std::vector<VkShaderModule> modules{};
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
	{
		// TODO: DON'T HARDCODE!!!
		// Vertex shader module.
		CreateShaderModule("../Vulkan-App-Framework/Assets/Shaders/core.UI.vert.spv", modules);
		// Fragment shader module.
		CreateShaderModule("../Vulkan-App-Framework/Assets/Shaders/core.UI.frag.spv", modules);

		// Vertex shader module.
		//CreateShaderModule("Assets/Shaders/core.UI.vert.spv", modules);
		// Fragment shader module.
		//CreateShaderModule("Assets/Shaders/core.UI.frag.spv", modules);

		shaderStages.resize(modules.size());
		for (int idx = 0; idx < modules.size(); idx++)
		{
			shaderStages[idx].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[idx].stage = (idx == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT; // TODO: Don't hardcode!
			shaderStages[idx].module = modules[idx];
			shaderStages[idx].pName = "main";
		}
	}

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// Vertex bindings
	VkVertexInputBindingDescription vertexBinding{};
	vertexBinding.binding = 0;
	vertexBinding.stride = sizeof(Vector2D);
	vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Vertex attributes
	std::vector<VkVertexInputAttributeDescription> vertexAttributes(1);

	// Attribute 1: position.
	vertexAttributes[0].location = 0;
	vertexAttributes[0].binding = 0;
	vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[0].offset = 0;

	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &vertexBinding;
	vertexInput.vertexAttributeDescriptionCount = 1;
	vertexInput.pVertexAttributeDescriptions = vertexAttributes.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	
	VkViewport viewport{};
	viewport.x = viewport.y = 0.0f;
	viewport.width = (float)m_Width;
	viewport.height = (float)m_Height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { m_Width, m_Height };

	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	// Multisampling.
	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // No multisampling for now.

	// Depth-Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	// Color-blending
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	// Color blending is disabled for now...
	colorBlendState.logicOpEnable = VK_FALSE;

	// Dynamic state
	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkDynamicState dynamicTypes[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamicState.pDynamicStates = dynamicTypes;
	dynamicState.dynamicStateCount = 2;

	// Push constants
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(uint32_t);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;	// TODO: make this specialized!

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = VK_NULL_HANDLE;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	VK_CHECK(vkCreatePipelineLayout(m_VulkanDevice.hLogicalDevice, &pipelineLayoutInfo, nullptr, &m_TestShader.pipelineLayout));

	pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInput;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = m_TestShader.pipelineLayout;

	VK_CHECK(vkCreateGraphicsPipelines(m_VulkanDevice.hLogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_TestShader.pipeline));

	// Release the modules.
	for (int idx = 0; idx < modules.size(); idx++)
	{
		vkDestroyShaderModule(m_VulkanDevice.hLogicalDevice, modules[idx], nullptr);
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation!
//-------------------------------------------------------------------------
void CVulkanRenderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.image = image;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage{};
	VkPipelineStageFlags destStage{};

	// TODO: Make cases for this based on the transition type!
	imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
	sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	destStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	VkImageAspectFlags aspectMask = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage,
						0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}


