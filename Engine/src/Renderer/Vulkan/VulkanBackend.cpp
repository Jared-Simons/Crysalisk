#include "VulkanBackend.h"

#include "Platform/Platform.h"
#include "Platform/VulkanPlatform.h"

#include "Core/Logging.h"
#include "Core/Event.h"

#include <Math/VertexTypes.h>

#include <String_Utils.h>
#include <Resources/ResourceTypes.h>

#include "Core/Input.h"
#include "Systems/TextureSystem.h" // TODO: Remove this dependency.

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

// TODO: This event callback really should be a part of the renderer backend class
bool Event_FramebufferResized(EventData_t data);
bool RendererEvent_KeyReleased(EventData_t data);

// TEMP: Camera
Camera s_Camera(Vector3D(0.0f, 0.0f, 3.0f), Vector3D(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
f32 deltaTime = 0.0f;
f32 lastFrame = 0.0f;

void UpdateCameraPosition(f32 deltaTime)
{
	f32 cameraSpeed = static_cast<f32>(2.5f * deltaTime);
	const f32 cameraRotationSpeedMultiplier = 20.0f;

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

	if (gpInputSystem->IsKeyPressed(KEY_RIGHT))
	{
		s_Camera.Yaw += cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_LEFT))
	{
		s_Camera.Yaw -= cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_UP))
	{
		s_Camera.Pitch += cameraSpeed * cameraRotationSpeedMultiplier;

		if (s_Camera.Pitch > 89.0f)
			s_Camera.Pitch = 89.0f;

		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_DOWN))
	{
		if (s_Camera.Pitch < -89.0f)
			s_Camera.Pitch = -89.0f;

		s_Camera.Pitch -= cameraSpeed * cameraRotationSpeedMultiplier;
		s_Camera.UpdateCameraVectors();
	}

	if (gpInputSystem->IsKeyPressed(KEY_P))
	{
		LOG_DEBUG("%.2f, %.2f, %.2f", s_Camera.Position.x, s_Camera.Position.y, s_Camera.Position.z);
	}

}


// END TEMP: Camera

bool CVulkanRenderer::RendererBackend_Initialize()
{
	// Cache off window dimensions here for later 
	// (it is VERY unlikely these will change again until the renderer is fully initialized)
	Platform::GetWindowSize(&m_FramebufferWidth, &m_FramebufferHeight);

	// Register for window resize events
	EventSystem::EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, Event_FramebufferResized);
	EventSystem::EventRegister(eEventCategory::INPUT_KEY_RELEASED, nullptr, RendererEvent_KeyReleased);

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
	for (i32 i = 0; i < m_CommandBuffers.size(); i++)
	{
		if (!VulkanCommandBuffer::CreateCommandBuffer(this, true, &m_CommandBuffers[i]))
			return false;
	}

	// Main shader
	if (!VulkanShader::CreateVulkanShader(this, &m_StandardShader))
		return false;

	// Swapchain framebuffers
	VulkanSwapchain::CreateFramebuffers(this, &m_Swapchain);

	// Buffers
	VkDeviceSize vertexBufferSize = sizeof(Vertex3D) * 1024;
	VkDeviceSize indexBufferSize = sizeof(u16) * 1024;

	if (!VulkanBuffer::CreateBuffer(this, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_VertexBuffer))
		return false;
	
	if (!VulkanBuffer::CreateBuffer(this, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_IndexBuffer))
		return false;

	// Fences
	// one per frame
	m_FrameInFlightFences.Resize(MAX_FRAMES_IN_FLIGHT);
	for (i32 i = 0; i < m_FrameInFlightFences.size(); i++)
	{
		if (!VulkanFence::CreateFence(this, true, &m_FrameInFlightFences[i]))
			return false;
	}

	// Semaphores
	// one per frame
	m_ImageAvailableSemaphores.Resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.Resize(MAX_FRAMES_IN_FLIGHT);
	for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (!VulkanSemaphore::CreateVulkanSemaphore(this, &m_ImageAvailableSemaphores[i]))
			return false;

		if (!VulkanSemaphore::CreateVulkanSemaphore(this, &m_RenderFinishedSemaphores[i]))
			return false;
	}

	m_RendererClock.Start();

	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());

	VulkanSwapchain::DestroySwapchain(this, &m_Swapchain);

	for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_RenderFinishedSemaphores[i]);
		VulkanSemaphore::DestroyVulkanSemaphore(this, &m_ImageAvailableSemaphores[i]);
	}

	for (i32 i = 0; i < m_FrameInFlightFences.size(); i++)
		VulkanFence::DestroyFence(this, &m_FrameInFlightFences[i]);

	VulkanBuffer::DestroyBuffer(this, &m_IndexBuffer);
	VulkanBuffer::DestroyBuffer(this, &m_VertexBuffer);

	for (i32 i = 0; i < m_CommandBuffers.size(); i++)
		VulkanCommandBuffer::DestroyCommandBuffer(this, &m_CommandBuffers[i]);

	VulkanCommandPool::DestroyCommandPool(this, &m_GraphicsCommandPool);

	VulkanShader::DestroyVulkanShader(this, &m_StandardShader);

	VulkanRenderpass::DestroyRenderpass(this, &m_MainRenderpass);

	VulkanDevice::DestroyDevice(this, &m_Device);
	
	vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
	DestroyDebugMessenger();

	delete m_ValidationLayers;

	vkDestroyInstance(m_Instance, m_Allocator);

	EventSystem::EventUnregister(eEventCategory::INPUT_KEY_RELEASED, nullptr, RendererEvent_KeyReleased);
	EventSystem::EventUnregister(eEventCategory::WINDOW_RESIZE, nullptr, Event_FramebufferResized);
}

bool CVulkanRenderer::RendererBackend_Update(struct RenderPacket_t* pRenderData)
{
	// Clear our metrics.
	m_Metrics.ResetMetrics();

	// Wait for the previous frame to finish it's work
	vkWaitForFences(m_Device.GetLogicalDevice(), 1, &m_FrameInFlightFences[m_CurrentFrame].m_Handle, VK_TRUE, UINT64_MAX);

	// TEMP
	// Update camera position

	f32 currentTime = static_cast<f32>(m_RendererClock.GetElapsedTime());
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;
	m_Metrics.deltaTime = deltaTime;

	UpdateCameraPosition(deltaTime);
	// END TEMP

	u32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device.GetLogicalDevice(), m_Swapchain.GetHandle(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame].m_Handle, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LOG_INFO("Swapchain out of date - Recreating.");
		m_Swapchain.RecreateSwapchain(this, g_NewWidth, g_NewHeight);
		return true;
	}

	vkResetFences(m_Device.GetLogicalDevice(), 1, &m_FrameInFlightFences[m_CurrentFrame].m_Handle);

	VulkanCommandBuffer& commandBuffer = m_CommandBuffers[m_CurrentFrame];

	if (!commandBuffer.GetHandle())
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

		m_StandardShader.Use(this, &commandBuffer.GetHandle(), imageIndex);

		// Apply globals
		m_StandardShader.BindGlobals(commandBuffer.GetHandle(), imageIndex);

		Matrix4x4 view = s_Camera.GetViewMatrix();
		Matrix4x4 proj = Matrix4x4::Perspective(M_DEG2RAD(45.0f), m_Swapchain.GetExtent().width / static_cast<f32>(m_Swapchain.GetExtent().height), 0.1f, 100.0f);
		proj.data[5] *= -1; // Flip the Y coordinate

		m_StandardShader.ApplyGlobals(this, commandBuffer.GetHandle(), imageIndex, proj, view);

		// Handle all draw calls here

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_Swapchain.GetExtent().width);
		viewport.height = static_cast<f32>(m_Swapchain.GetExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer.GetHandle(), 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_Swapchain.GetExtent();
		vkCmdSetScissor(commandBuffer.GetHandle(), 0, 1, &scissor);

		for (i32 i = 0; i < 10; i++)
		{
			if (pRenderData->geometries[i] && pRenderData->geometries[i]->m_pinternalData)
			{
				DrawGeometry(*pRenderData->geometries[i], m_VertexBuffer, m_IndexBuffer, commandBuffer, imageIndex);

				// Update render metrics.
				{
					m_Metrics.numDrawCalls++;
					m_Metrics.numMeshes++;
					m_Metrics.numVertices = 36; // TODO: Hardcoded
					m_Metrics.numTriangles = 12;
				}
			}
		}

		m_MainRenderpass.End(this, &commandBuffer);

	commandBuffer.End();

	// Submit the command buffer for execution
	commandBuffer.Submit(this);

	// Present the framebuffer
	m_Swapchain.Present(this, imageIndex);
	commandBuffer.SetState(eVulkanCommandBufferState::INVALID);

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

	instanceInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions.size());
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

		instanceInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers->size());
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
		for (i32 i = 0; i < requiredExtensions.size(); i++)
		{
			LOG_DEBUG("%s", requiredExtensions[i]);
		}

		LOG_INFO("Enabled Validation Layers:");
		
		for (i32 i = 0; i < m_ValidationLayers->size(); i++)
		{
			LOG_DEBUG("%s", m_ValidationLayers->data()[i]);
		}
	}

	LOG_INFO("Vulkan Instance Created");
	return true;
}

bool CVulkanRenderer::QueryInstanceExtensionSupport(CVector<const char*>& requiredExtensions)
{
	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	CVector<VkExtensionProperties> instanceExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

	for (u32 i = 0; i < requiredExtensions.size(); i++)
	{
		bool bFound = false;
		for (u32 j = 0; j < extensionCount; j++)
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
	u32 layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	CVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	for (u32 i = 0; i < requiredValidationLayers.size(); i++)
	{
		bool bFound = false;
		for (u32 j = 0; j < layerCount; j++)
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

bool CVulkanRenderer::CreateTextureResources(Texture& pTexture, byte* pPixelData)
{
	// Load the pixel data into a staging buffer.
	VulkanBuffer stagingBuffer;
	VkDeviceSize imageSize = pTexture.m_Width * pTexture.m_Height * 4;
	VulkanBuffer::CreateBuffer(this, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

	void* pImageData;
	vkMapMemory(m_Device.GetLogicalDevice(), stagingBuffer.m_Memory, 0, imageSize, 0, &pImageData);
		Memory::MemCopy(pImageData, pPixelData, static_cast<u64>(imageSize));
	vkUnmapMemory(m_Device.GetLogicalDevice(), stagingBuffer.m_Memory);

	pTexture.m_internalData = Memory::Allocate(sizeof(VulkanImageData_t), eMemAllocCategories::MEM_CATEGORY_UNKNOWN);
	VulkanImageData_t* pInternalData = reinterpret_cast<VulkanImageData_t*>(pTexture.m_internalData);
	VulkanImage* pInternalImage = &pInternalData->internalImage;

	VulkanImage::CreateImage(this, pTexture.m_Width, pTexture.m_Height, 4, nullptr, true, pInternalImage);

	pInternalImage->TransitionImageLayout(this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		stagingBuffer.CopyBufferToImage(this, pTexture.m_Width, pTexture.m_Height, pInternalImage);
	pInternalImage->TransitionImageLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Release the staging buffer.
	VulkanBuffer::DestroyBuffer(this, &stagingBuffer);

	// Acquire shader resources.
	if (!m_StandardShader.AcquireInstanceResources(this, pTexture))
	{
		LOG_ERROR("Failed to acquire Vulkan resources for texture.");
		return false;
	}

	return true;
}

void CVulkanRenderer::DestroyTextureResources(Texture& pTexture)
{
	// Make sure this resource is not being used!
	// TODO: Is this the most efficient way of doing this?
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());
	
	if (pTexture.m_internalData)
	{
		VulkanImage* pTextureData = reinterpret_cast<VulkanImage*>(pTexture.m_internalData);

		if (pTextureData)
			VulkanImage::DestroyImage(this, pTextureData);

		// TODO: Release internal shader instance resources as well

		Memory::Free(pTexture.m_internalData, sizeof(VulkanImageData_t), eMemAllocCategories::MEM_CATEGORY_UNKNOWN);
		pTexture.m_internalData = nullptr;
	}
}

bool CVulkanRenderer::CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices)
{
	pMesh.m_pinternalData = Memory::Allocate(sizeof(VulkanGeometryData_t), eMemAllocCategories::MEM_CATEGORY_ARRAY);
	VulkanGeometryData_t* internalData = reinterpret_cast<VulkanGeometryData_t*>(pMesh.m_pinternalData);

	VkDeviceSize vertexDataSize = sizeof(vertices[0]) * vertices.size();
	VkDeviceSize indexDataSize = sizeof(indices[0]) * indices.size();

	// Store off our offsets for access later.
	internalData->m_VertexDataOffset = m_VertexBufferOffset;
	internalData->m_IndexDataOffset = m_IndexBufferOffset;

	internalData->m_VertexDataSize = static_cast<VkDeviceSize>(vertexDataSize);
	internalData->m_IndexDataSize = static_cast<VkDeviceSize>(indexDataSize);

	// Upload the data to the buffers
	m_VertexBuffer.UploadDataToBuffer(this, vertexDataSize, vertices.data(), m_VertexBufferOffset);
	m_VertexBufferOffset += vertexDataSize;

	m_IndexBuffer.UploadDataToBuffer(this, indexDataSize, indices.data(), m_IndexBufferOffset);
	m_IndexBufferOffset += indexDataSize;

	return true;
}

void CVulkanRenderer::DestroyGeometryResources(StaticMesh& pMesh)
{
	// TODO: Implement with freelist

	//gpMemorySystem->MemFree(pMesh.m_pinternalData, sizeof(VulkanGeometryData_t), eMemAllocCategories::MEM_CATEGORY_ARRAY);
	//pMesh.m_pinternalData = nullptr;
}

void CVulkanRenderer::DrawGeometry(StaticMesh& pMesh, VulkanBuffer& pVertexBuffer, VulkanBuffer& pIndexBuffer, VulkanCommandBuffer& pCommandBuffer, u32 imageIndex)
{
	// TODO: Material-specific shaders
	// TODO: Clean up this texture check
	if (!pMesh.m_pTexture->m_internalData)
	{
		m_StandardShader.BindInstance(pCommandBuffer.GetHandle(), imageIndex, *gpTextureSystem->GetDefaultDiffuseTexture());
		m_StandardShader.ApplyInstance(this, pCommandBuffer.GetHandle(), imageIndex, *gpTextureSystem->GetDefaultDiffuseTexture());
	}
	else
	{
		m_StandardShader.BindInstance(pCommandBuffer.GetHandle(), imageIndex, *pMesh.m_pTexture);
		m_StandardShader.ApplyInstance(this, pCommandBuffer.GetHandle(), imageIndex, *pMesh.m_pTexture);
	}

	// Apply model matrix
	Matrix4x4 model = Matrix4x4::Translate(Matrix4x4(1.0f), Vector3D(0.0f, 0.0f, 0.0f));
	static f64 startTime = Platform::GetAbsoluteTime();
	f64 currentTime = Platform::GetAbsoluteTime();

	f64 time = currentTime - startTime;
	//model = Matrix4x4::Rotate(Matrix4x4(1.0f), time * M_DEG2RAD(45.0f), Vector3D(0.0f, 1.0f, 0.0f));

	m_StandardShader.ApplyModel(pCommandBuffer.GetHandle(), model);

	VulkanGeometryData_t* internalData = reinterpret_cast<VulkanGeometryData_t*>(pMesh.m_pinternalData);

	VkDeviceSize vertexOffset = internalData->m_VertexDataOffset;
	VkDeviceSize indexOffset = internalData->m_IndexDataOffset;

	u64 numVertices = internalData->m_VertexDataSize / sizeof(Vertex3D);
	u64 numIndices = internalData->m_IndexDataSize / sizeof(u16);

	vkCmdBindVertexBuffers(pCommandBuffer.GetHandle(), 0, 1, &pVertexBuffer.m_Handle, &vertexOffset);

	// TODO: Implement
	// If we are able to draw with indices, do so.
	/*if (internalData->m_IndexDataSize != 0)
	{
		vkCmdBindIndexBuffer(pCommandBuffer.m_Handle, pIndexBuffer.m_Handle, indexOffset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(pCommandBuffer.m_Handle, numIndices, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(pCommandBuffer.m_Handle, numVertices, 1, 0, 0);
	}*/

	vkCmdDraw(pCommandBuffer.GetHandle(), static_cast<u32>(numVertices), 1, 0, 0);
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
	u32 width, height;
	width = data.u16[0];
	height = data.u16[1];

	g_NewWidth = width;
	g_NewHeight = height;
	g_FramebufferResized = true;

	return true;
}

bool RendererEvent_KeyReleased(EventData_t data)
{
	char keycode = *data.c;

	if (keycode == KEY_R)
	{
		// Print out current rendering metrics.
		char renderingMetricsBuffer[4096];
		StringFormat_V("Rendering Metrics\nTriangles:\t%d\nVertices:\t%d\nMeshes:\t\t%d\nDraw Calls:\t%d", nullptr, renderingMetricsBuffer, 
																										s_VulkanRenderer->m_Metrics.numTriangles, 
																										s_VulkanRenderer->m_Metrics.numVertices, 
																										s_VulkanRenderer->m_Metrics.numMeshes, 
																										s_VulkanRenderer->m_Metrics.numDrawCalls);

		LOG_DEBUG(renderingMetricsBuffer);
	}

	return false;
}

