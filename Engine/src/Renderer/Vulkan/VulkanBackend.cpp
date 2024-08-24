#include "VulkanBackend.h"

#include "Platform/Platform.h"
#include "Platform/VulkanPlatform.h"

#include "Core/Logging.h"
#include "Systems/IEvent.h"

#include <Math/VertexTypes.h>

#include <String_Utils.h>



#include "Systems/IInput.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

// TODO: This event callback really should be a part of the renderer backend class
bool Event_FramebufferResized(EventData_t data);

// TEMP: Camera
Camera s_Camera(Vector3D(0.0f, 0.0f, 3.0f), Vector3D(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void UpdateCameraPosition(float deltaTime)
{
	float cameraSpeed = static_cast<float>(2.5f * deltaTime);
	const float cameraRotationSpeedMultiplier = 20.0f;

	if (gpInputSystem->IsKeyPressed(KEY_W))
		s_Camera.Position = s_Camera.Position + (s_Camera.Front * cameraSpeed);
	if (gpInputSystem->IsKeyPressed(KEY_S))
		s_Camera.Position = s_Camera.Position - (s_Camera.Front * cameraSpeed);
	if (gpInputSystem->IsKeyPressed(KEY_A))
	{
		Vector3D newVector = (Vector3D::CrossProduct(s_Camera.Front, s_Camera.Up));
		s_Camera.Position = s_Camera.Position - (newVector.Normalize() * cameraSpeed);
	}
	if (gpInputSystem->IsKeyPressed(KEY_D))
	{
		Vector3D newVector = (Vector3D::CrossProduct(s_Camera.Front, s_Camera.Up));
		s_Camera.Position = s_Camera.Position + (newVector.Normalize() * cameraSpeed);
	}

	if (gpInputSystem->IsKeyPressed(KEY_SPACE))
		s_Camera.Position = s_Camera.Position + (s_Camera.Up * cameraSpeed);

	if (gpInputSystem->IsKeyPressed(KEY_CTRL))
		s_Camera.Position = s_Camera.Position - (s_Camera.Up * cameraSpeed);

	if (gpInputSystem->IsKeyPressed(KEY_E))
	{
		s_Camera.Yaw += cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_Q))
	{
		s_Camera.Yaw -= cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_UP))
	{
		s_Camera.Pitch += cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_DOWN))
	{
		s_Camera.Pitch -= cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

}


// END TEMP: Camera

bool CVulkanRenderer::RendererBackend_Initialize()
{
	// Cache off window dimensions here for later 
	// (it is VERY unlikely these will change again until the renderer is fully initialized)
	Platform::GetWindowSize(&m_FramebufferWidth, &m_FramebufferHeight);

	// Register for window resize events
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, Event_FramebufferResized);

	// Instance
	if (!CreateVulkanInstance())
		return false;

	// Debug Messenger
	if (!CreateDebugMessenger())
		return false;

	// Surface
	if (!Platform::CreateVulkanSurface(this))
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

	// Main shader
	if (!VulkanShader::CreateVulkanShader(this, &m_StandardShader))
		return false;

	// Framebuffers
	// one per frame
	m_Swapchain.m_Framebuffers.Resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < m_Swapchain.m_Framebuffers.size(); i++)
	{
		if (!VulkanFramebuffer::CreateFramebuffer(this, &m_Swapchain.m_Framebuffers[i], i))
			return false;
	}

	// Buffers

	// TODO: Temp rendering data
	
	CVector<Vertex3D> vertices(36);
	/*vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.0f), Vector3D(1.0, 0.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.0f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.0f), Vector3D(0.0, 0.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.0f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));*/

	CVector<uint16_t> indices((size_t)6);
	indices.PushBack(0);
	indices.PushBack(1);
	indices.PushBack(3);
	indices.PushBack(1);
	indices.PushBack(2);
	indices.PushBack(3);

	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));

	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	
	
	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.capacity();
	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.capacity();

	if (!VulkanBuffer::CreateBuffer(this, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_VertexBuffer))
		return false;
	
	if (!VulkanBuffer::CreateBuffer(this, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_IndexBuffer))
		return false;

	// Upload the data to the GPU
	m_VertexBuffer.UploadDataToBuffer(this, vertexBufferSize, vertices.data());
	m_IndexBuffer.UploadDataToBuffer(this, indexBufferSize, indices.data());

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

	LOG_DEBUG("%.2f %.2f %.2f ", s_Camera.Position.x, s_Camera.Position.y, s_Camera.Position.z);
	m_RendererClock.Start();

	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	vkDeviceWaitIdle(m_Device.m_LogicalDevice);

	VulkanSwapchain::DestroySwapchain(this, &m_Swapchain);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_RenderFinishedSemaphores[i]);
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_ImageAvailableSemaphores[i]);
	}

	for (int i = 0; i < m_FrameInFlightFences.size(); i++)
		VulkanFence::DestroyFence(this, &m_FrameInFlightFences[i]);

	VulkanBuffer::DestroyBuffer(this, &m_IndexBuffer);
	VulkanBuffer::DestroyBuffer(this, &m_VertexBuffer);

	for (int i = 0; i < m_Swapchain.m_Framebuffers.size(); i++)
		VulkanFramebuffer::DestroyFramebuffer(this, &m_Swapchain.m_Framebuffers[i]);

	for (int i = 0; i < m_CommandBuffers.size(); i++)
		VulkanCommandBuffer::DestroyCommandBuffer(this, &m_CommandBuffers[i]);

	VulkanCommandPool::DestroyCommandPool(this, &m_GraphicsCommandPool);

	VulkanShader::DestroyVulkanShader(this, &m_StandardShader);

	VulkanRenderpass::DestroyRenderpass(this, &m_MainRenderpass);

	VulkanDevice::DestroyDevice(this, &m_Device);
	
	vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
	DestroyDebugMessenger();

	delete m_ValidationLayers;

	vkDestroyInstance(m_Instance, m_Allocator);

	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, Event_FramebufferResized);
}

bool CVulkanRenderer::RendererBackend_Update()
{
	// Wait for the previous frame to finish it's work
	vkWaitForFences(m_Device.m_LogicalDevice, 1, &m_FrameInFlightFences[m_CurrentFrame].m_Handle, VK_TRUE, UINT64_MAX);

	// TEMP
	// Update camera position

	float currentTime = static_cast<float>(m_RendererClock.GetElapsedTime());
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;

	UpdateCameraPosition(deltaTime);
	// END TEMP

	unsigned int imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device.m_LogicalDevice, m_Swapchain.m_SwapchainHandle, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame].m_Handle, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LOG_INFO("Swapchain out of date - Recreating.");
		m_Swapchain.RecreateSwapchain(this, g_NewWidth, g_NewHeight);
		return true;
	}

	// Update Uniform Buffer
	m_StandardShader.UpdateUniforms(this, imageIndex, s_Camera, deltaTime);

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

		// Handle all draw calls here

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_Swapchain.m_SwapchainExtent.width);
		viewport.height = static_cast<float>(m_Swapchain.m_SwapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer.m_Handle, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_Swapchain.m_SwapchainExtent;
		vkCmdSetScissor(commandBuffer.m_Handle, 0, 1, &scissor);

		m_StandardShader.Bind(this, &commandBuffer.m_Handle, imageIndex);

		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer.m_Handle, 0, 1, &m_VertexBuffer.m_Handle, offset);

		// If we are able to draw with indices, do so.
		/*if (!m_IndexBuffer.m_Handle)
		{
			vkCmdDraw(commandBuffer.m_Handle, 3, 1, 0, 0);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer.m_Handle, m_IndexBuffer.m_Handle, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(commandBuffer.m_Handle, 6, 1, 0, 0, 0);
		}*/

		vkCmdDraw(commandBuffer.m_Handle, 36, 1, 0, 0);

		m_MainRenderpass.End(this, &commandBuffer);

	commandBuffer.End();

	// Submit the command buffer for execution
	commandBuffer.Submit(this);

	// Present the framebuffer
	m_Swapchain.Present(this, imageIndex);
	commandBuffer.m_State = eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INVALID;

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
	requiredExtensions.PushBack(Platform::GetVulkanExtensions());

	if (EnableValidationLayers())
	{
		requiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (!QueryInstanceExtensionSupport(requiredExtensions))
	{
		LOG_ERROR("Required Vulkan instance extensions were not found - Application cannot continue.");
		return false;
	}

	instanceInfo.enabledExtensionCount = static_cast<unsigned int>(requiredExtensions.size());
	instanceInfo.ppEnabledExtensionNames = static_cast<const char**>(requiredExtensions.data());

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

		instanceInfo.enabledLayerCount = static_cast<unsigned int>(m_ValidationLayers->size());
		instanceInfo.ppEnabledLayerNames = static_cast<const char**>(m_ValidationLayers->data());
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

bool Event_FramebufferResized(EventData_t data)
{
	unsigned int width, height;
	width = data.u16[0];
	height = data.u16[1];

	g_NewWidth = width;
	g_NewHeight = height;
	g_FramebufferResized = true;

	return true;
}

