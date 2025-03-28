//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#include "VulkanRenderer.h"

#include <Core/Event.h>
#include <Core/Input.h>
#include <Core/Logging.h>
#include <Defines.h>
#include <Interfaces.h>
#include <Math/MathLib.h>
#include <Math/VectorTypes.h>
#include <Math/VertexTypes.h>
#include <Platform/FileSystem.h>
#include <Platform/InterfaceRegistry.h>
#include <Renderer/RendererTypes.h>
#include <Resources/GeometryGenerator.h>
#include <Resources/ResourceTypes.h>
#include <Strings/String_Utils.h>

// Vulkan Renderer Context.
static CVulkanRenderer s_VulkanContext;
IRendererBackend* g_pRendererBackend = &s_VulkanContext;

// Forward Declarations
static bool QueryValidationLayerSupport(const Utl::CVector<const char*>& expectedLayers);
static bool QueryInstanceExtensionSupport(const Utl::CVector<const char*>& expectedExtensions);
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

static bool RendererEvent_WindowResized(EventData_t data);
static bool RendererEvent_KeyPressed(EventData_t data);

// TEMPORARY CAMERA DATA
// THIS SHOULD BE MOVED ELSEWHERE EVENTUALLY!
struct Camera_t
{
	Vector3D vecPosition{ 0, 0, 3 };
	Vector3D vecFront{ 0, 0, -1 };
	Vector3D vecRight{ 1, 0, 0 };
	Vector3D vecUp{ 0, 1, 0 };
	Vector3D vecWorldUp{ 0, 1, 0 };
	
	float	Yaw{ -90.0f };
	float	Pitch{ 0.0f };
	float	MoveSpeed{ 2.5f };
	float	TurnSpeed{ 0.5f };

	inline Matrix4x4 GetViewMatrix()
	{
		return Matrix4x4::LookAt(vecPosition, vecPosition + vecFront, vecUp);
	}

	void RecalculateCameraVectors()
	{
		Vector3D front{};
		front.x = M_COS_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
		front.y = M_SIN_F(M_DEG2RAD_F(Pitch));
		front.z = M_SIN_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
		vecFront = front.Normalize();

		Vector3D newRight = Vector3D::CrossProduct(vecFront, vecWorldUp);
		vecRight = newRight.Normalize();

		Vector3D newUp = Vector3D::CrossProduct(vecRight, vecFront);
		vecUp = newUp.Normalize();
	}

	void UpdateCameraData(double deltaTime)
	{
		float velocity = (float)(2.5f * deltaTime);
		const float cameraSpeedMultiplier = 50.0;
		const float cameraRotationSpeedMultipler = 50.0f;

		if (g_pInputSystem->IsKeyPressed(KEY_W))
		{
			vecPosition += (vecFront * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_S))
		{
			vecPosition -= (vecFront * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_A))
		{
			vecPosition -= (vecRight * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_D))
		{
			vecPosition += (vecRight * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_SPACE))
		{
			vecPosition += (vecUp * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_CTRL))
		{
			vecPosition -= (vecUp * (velocity * cameraSpeedMultiplier));
		}
		if (g_pInputSystem->IsKeyPressed(KEY_RIGHT))
		{
			Yaw += velocity * cameraRotationSpeedMultipler;
			RecalculateCameraVectors();
		}
		if (g_pInputSystem->IsKeyPressed(KEY_LEFT))
		{
			Yaw -= velocity * cameraRotationSpeedMultipler;
			RecalculateCameraVectors();
		}
		if (g_pInputSystem->IsKeyPressed(KEY_UP))
		{
			Pitch += velocity * cameraRotationSpeedMultipler;

			if (Pitch > 89.0f)
				Pitch = 89.0f;

			RecalculateCameraVectors();
		}
		if (g_pInputSystem->IsKeyPressed(KEY_DOWN))
		{
			Pitch -= velocity * cameraRotationSpeedMultipler;

			if (Pitch < -89.0f)
				Pitch = -89.0f;

			RecalculateCameraVectors();
		}
	}
};

static Camera_t s_MainCamera;

// END TEMP CAMERA

//-------------------------------------------------------------------------
// Purpose: Gets opaque pointers to interfaces that will be exposed to the rest of the engine.
// Input: none.
// Output: An array of pointers and names to interfaces this library will provide.
//-------------------------------------------------------------------------
DLL_EXPORT Utl::CVector<Platform::SystemInterface_t> ObtainInterfaces()
{
	Utl::CVector<Platform::SystemInterface_t> interfaces{};

	// For this, right now we only care about the backend pointer.
	interfaces.PushBack({ g_pRendererBackend, "Renderer_Backend" });

	return interfaces;
}

//-------------------------------------------------------------------------
// Purpose: Connect interfaces provided by other libraries and do any work to make sure they're present.
// Input: pInterfaceState - An array of interfaces to be setup.
// Output: none.
//-------------------------------------------------------------------------
DLL_EXPORT void ConnectInterface(Utl::CVector<Platform::SystemInterface_t>& pInterfaceState)
{
	// TODO: Grab and assign interfaces needed by this module here!
	AssignInterfaces(pInterfaceState);

	// Register for window resize event.
	if (g_pEngineEvents)
	{
		g_pEngineEvents->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);
	}
}

//-------------------------------------------------------------------------
// Purpose: Returns an opaque pointer to the renderer backend state.
//-------------------------------------------------------------------------
DLL_EXPORT void* GetRendererBackendState()
{
	return g_pRendererBackend;
}

//-------------------------------------------------------------------------
// Purpose: Initializes the Vulkan renderer.
// Input: pConfig - Renderer specific configuration provided by the calling application.
// Output: bool - Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::RendererBackend_Initialize(RendererConfig_t& pConfig)
{
	// Setup Vulkan Instance.
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = pConfig.szApplicationName;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0); 
	appInfo.pEngineName = "Crysalisk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0); // We're using Vulkan 1.3 for now.

	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &appInfo;

	// Validation Layers.
	Utl::CVector<const char*> validationLayers;

#ifdef _DEBUG
	validationLayers.PushBack("VK_LAYER_KHRONOS_validation");

	if (!QueryValidationLayerSupport(validationLayers))
	{
		LOG_ERROR("Required Vulkan validation layers were not found.");
		return false;
	}

	LOG_DEBUG("Vulkan validation layers found!");

	instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.Count());
	instanceInfo.ppEnabledLayerNames = validationLayers.Data();
#else
	instanceInfo.enabledLayerCount = 0;
	instanceInfo.ppEnabledLayerNames = nullptr;
#endif

	// Instance extensions.
	Utl::CVector<const char*> instanceExtensions(2);
	instanceExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
	instanceExtensions.PushBack(Platform::GetVulkanPlatformExtension());

#ifdef _DEBUG
		instanceExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	if (!QueryInstanceExtensionSupport(instanceExtensions))
	{
		LOG_ERROR("Required Vulkan instance extensions were not found - Application cannot continue.");
		return false;
	}

	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.Count());
	instanceInfo.ppEnabledExtensionNames = instanceExtensions.Data();
	
	VK_CHECK(vkCreateInstance(&instanceInfo, m_Allocator, &m_Instance));
	LOG_DEBUG("Vulkan instance initialized");

	// Debug messenger
#ifdef _DEBUG
	// Debug callbacks only exist in debug builds.
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = DebugCallbackFunc;

	PFN_vkCreateDebugUtilsMessengerEXT createDebuggerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (createDebuggerFunc != nullptr)
	{
		VK_CHECK(createDebuggerFunc(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger));
	}
	else
	{
		LOG_ERROR("Failed to create debug messenger (VK_ERROR_EXTENSION_NOT_PRESENT)");
		return false;
	}

	LOG_DEBUG("Vulkan debug messenger created");
#endif

	// Surface
	if (!Platform::CreateVulkanSurface(s_VulkanContext, pConfig.pHWnd))
	{
		LOG_ERROR("Failed to create Vulkan surface. Renderer initialization failed!");
		return false;
	}

	// Device
	if (!VulkanDevice::CreateDevice(s_VulkanContext, m_Device))
	{
		LOG_ERROR("Failed to initialize Vulkan device. Renderer initialization failed!");
		return false;
	}

	// Graphics Command Pool
	VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // We need to be able to reset our command buffers manually for now.
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = m_Device.m_QueueFamilyIndices.GraphicsQueueFamilyIndex;

	VK_CHECK(vkCreateCommandPool(m_Device.m_Device, &commandPoolInfo, m_Allocator, &m_GraphicsCommandPool));

	// Allocate Graphics Command Buffers
	m_GraphicsCommandBuffers.Resize(m_MaxImagesInFlight);
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		if (!VulkanCommandBuffer::CreateCommandBuffer(s_VulkanContext, true, m_GraphicsCommandBuffers[idx]))
		{
			LOG_ERROR("Failed to create Vulkan Command Buffer!");
			return false;
		}
	}

	// Swapchain
	if (!VulkanSwapchain::CreateSwapchain(s_VulkanContext, m_Swapchain))
	{
		LOG_ERROR("Failed to initialize Vulkan Swapchain. Renderer initialization failed!");
		return false;
	}

	// Main Renderpass
	if (!VulkanRenderPass::CreateRenderpass(s_VulkanContext, m_MainRenderPass))
	{
		LOG_ERROR("Failed to initialize main Vulkan Renderpass. Renderer initialization failed!");
		return false;
	}

	// Swapchain Framebuffers
	if (!m_Swapchain.CreateFramebuffers(s_VulkanContext, m_Swapchain))
	{
		LOG_ERROR("Failed to create swapchain framebuffers. Renderer initialization failed!");
		return false;
	}

	// Create synchronization primitives.
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create our fence in a 'signaled' state so the first frame won't wait on it.

	VK_CHECK(vkCreateSemaphore(m_Device.m_Device, &semaphoreInfo, m_Allocator, &m_ImageAvailableSemaphore));
	VK_CHECK(vkCreateSemaphore(m_Device.m_Device, &semaphoreInfo, m_Allocator, &m_RenderFinishedSemaphore));
	VK_CHECK(vkCreateFence(m_Device.m_Device, &fenceInfo, m_Allocator, &m_InFlightFence));

	// Create Global Descriptor Pool
	VkDescriptorPoolSize poolSizes[2]{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = (uint32_t)(m_MaxImagesInFlight + (3 * kMaxVulkanInstances));		// Enough for global uniforms and max object instances.

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 10 + ((int)eTextureType::MAX_TEXTURE_TYPES * kMaxVulkanInstances);									// Enough for 10 global textures and kMaxVulkanInstances of materials.

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;
	descriptorPoolInfo.maxSets = (uint32_t)(poolSizes[0].descriptorCount + poolSizes[1].descriptorCount);

	VK_CHECK(vkCreateDescriptorPool(m_Device.m_Device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));

	// Set up our global uniform buffers.
	m_UniformBuffers.Resize(m_MaxImagesInFlight);
	for (int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		if (!VulkanBuffer::CreateBuffer(s_VulkanContext, (sizeof(GlobalUniformObject_t)) + (sizeof(MaterialInstanceData_t) * kMaxVulkanInstances), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[idx]))
		{
			LOG_ERROR("Failed to create uniform buffer.");
			return false;
		}
	}

	// Create the temp geometry buffer.
	if (!VulkanBuffer::CreateBuffer(s_VulkanContext, sizeof(Vertex3D) * 20 * 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_GeometryVertexBuffer))
	{
		LOG_ERROR("Failed to create geometry vertex data buffer.");
		return false;
	}

	if (!VulkanBuffer::CreateBuffer(s_VulkanContext, sizeof(uint32_t) * 20 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_GeometryIndexBuffer))
	{
		LOG_ERROR("Failed to create geometry index data buffer.");
		return false;
	}

	// Allocate resources for our image data.
	m_pInternalTextureData = (VulkanTextureInternal*)new VulkanTextureInternal[pConfig.MaxTextures];

	// Invalidate all entries.
	for (int idx = 0; idx < pConfig.MaxTextures; idx++)
	{
		m_pInternalTextureData[idx].m_Generation = kInvalidID_U8;
	}

	// Allocate resources for our shaders.
	m_pInternalShaderData = new VulkanShaderInternal[pConfig.MaxPipelines];

	// Save off our configuration.
	m_RendererConfig = pConfig;

	// Register for input events.
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);

	s_MainCamera.RecalculateCameraVectors();

	m_RendererClock.Start();

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the Vulkan renderer.
// Input: none.
// Output: none.
//-------------------------------------------------------------------------
void CVulkanRenderer::RendererBackend_Shutdown()
{
	// Make sure none of our resources are in use.
	vkDeviceWaitIdle(m_Device.m_Device);

	// Destroy our temp geometry.
	// THIS IS TEMPORARY
	DestroyDefaultGeometry();

	// Release uniform buffer resources.
	for (int idx = 0; idx < m_MaxImagesInFlight; idx++)
		VulkanBuffer::DestroyBuffer(s_VulkanContext, m_UniformBuffers[idx]);

	// Free internal texture/shader data.
	delete[] m_pInternalShaderData;
	delete[] m_pInternalTextureData;

	// Descriptor Pool
	vkDestroyDescriptorPool(m_Device.m_Device, m_DescriptorPool, m_Allocator);

	// Sync objects.
	vkDestroyFence(m_Device.m_Device, m_InFlightFence, m_Allocator);
	vkDestroySemaphore(m_Device.m_Device, m_RenderFinishedSemaphore, m_Allocator);
	vkDestroySemaphore(m_Device.m_Device, m_ImageAvailableSemaphore, m_Allocator);

	// Main Renderpass
	VulkanRenderPass::DestroyRenderpass(s_VulkanContext, m_MainRenderPass);

	// Swapchain
	VulkanSwapchain::DestroySwapchain(s_VulkanContext, m_Swapchain);

	// Graphics Command Pool
	vkDestroyCommandPool(m_Device.m_Device, m_GraphicsCommandPool, m_Allocator);

	// Device
	VulkanDevice::DestroyDevice(s_VulkanContext, m_Device);

#ifdef _DEBUG
	// Debug messenger
	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebuggerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

	if (destroyDebuggerFunc != nullptr)
	{
		destroyDebuggerFunc(m_Instance, m_DebugMessenger, m_Allocator);
		return;
	}
#endif

	// Instance
	vkDestroyInstance(m_Instance, m_Allocator);
}

//-------------------------------------------------------------------------
// Purpose: Begins a renderer frame.
// Input: pRenderData - Frame specific render data provided by the application.
// Output: bool - Returns true on successful start, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::StartFrame(RenderPacket_t& pRenderData)
{
	if (m_bResizingFramebuffer)
	{
		return true;
	}

	// Wait for the prior frame to finish it's work.
	vkWaitForFences(m_Device.m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

	// TODO: There should be a clock at the application level, with a global delta time passed in with the render packet.
	double currentFrameTime = m_RendererClock.GetElapsedTime();
	m_flDeltaTime = currentFrameTime - m_flLastFrameTime;
	m_flLastFrameTime = currentFrameTime;

	s_MainCamera.UpdateCameraData(m_flDeltaTime);

	//LOG_DEBUG("Delta Time: %.8f", currentFrameTime);

	// Acquire the next swapchain image.
	VkResult imageAcquireResult = vkAcquireNextImageKHR(m_Device.m_Device, m_Swapchain.m_Handle, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_ImageIndex);

	CAssert(!m_bCurrentlyRendering[m_ImageIndex]);
	m_bCurrentlyRendering[m_ImageIndex] = true;

	if (imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(m_Device.m_Device);
		// We need to recreate our swapchain.
		m_Swapchain.RecreateSwapchain(s_VulkanContext, m_Swapchain);

		m_bWindowResized = false;
		return true;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR)
	{
		// If we get in here it is very likely that we won't be able to recover...
		CAssert(0);
		return false;
	}

	// Only reset the fence if we got an image to work with.
	vkResetFences(m_Device.m_Device, 1, &m_InFlightFence);

	// Reset and startup our current command buffer.
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;
	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo));

	// Start the renderpass and update globals.
	VkRenderPassBeginInfo renderpassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderpassInfo.renderPass = m_MainRenderPass.m_Handle;
	renderpassInfo.framebuffer = m_Swapchain.m_Framebuffers[m_ImageIndex];
	renderpassInfo.renderArea.offset = { 0, 0 };
	renderpassInfo.renderArea.extent = m_Swapchain.m_SwapchainExtent;

	VkClearValue clearColors[2] = { {0.3f, 0.0f, 0.3f, 1.0f}, {1.0f, 0} };
	renderpassInfo.clearValueCount = 2;
	renderpassInfo.pClearValues = clearColors;

	vkCmdBeginRenderPass(commandBuffer, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set our dynamic state.
	// TODO: Should this be with the pipeline bind?
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_Swapchain.m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_Swapchain.m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain.m_SwapchainExtent;

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	m_UniformData.View = s_MainCamera.GetViewMatrix();
	m_UniformData.Projection = Matrix4x4::Perspective(M_DEG2RAD_F(45.0f), m_Swapchain.m_SwapchainExtent.width / (float)m_Swapchain.m_SwapchainExtent.height, 0.1f, 10000000.0f);
	m_UniformData.Projection.data[5] *= -1;

	m_UniformData.ViewPos = s_MainCamera.vecPosition;

	if (m_UniformData.RenderMode != m_RenderMode)
		m_UniformData.RenderMode = m_RenderMode;

	double lightSin = M_SIN(Platform::GetAbsoluteTime() * 1.5);
	double lightCos = M_COS(Platform::GetAbsoluteTime() * 1.5);
	m_UniformData.DirLightPos.x = lightCos * 300;
	m_UniformData.DirLightPos.z = lightSin * 300;
	m_UniformData.DirLightPos.y = 100.0f;

	//LOG_INFO("%f, %f", m_UniformData.DirLightPos.x, m_UniformData.DirLightPos.z);

	// Copy to the GPU.
	void* pUniformData;
	VkDeviceSize uniformOffset = { 0 };
	vkMapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory, uniformOffset, sizeof(GlobalUniformObject_t), 0, &pUniformData);
	memcpy(pUniformData, &m_UniformData, sizeof(GlobalUniformObject_t));
	vkUnmapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Ends a renderer frame.
// Input: pRenderData - Frame specific render data provided by the application.
// Output: bool - Returns true on successful end, otherwise false.
//-------------------------------------------------------------------------
void CVulkanRenderer::EndFrame(RenderPacket_t& pRenderData)
{
	if (m_bResizingFramebuffer)
	{
		return;
	}

	CAssert(m_bCurrentlyRendering[m_ImageIndex]);

	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;
	vkCmdEndRenderPass(commandBuffer);

	// End and submit our command buffer.
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo cmdSubmitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore }; // We'll be waiting on the for the image to become available.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	cmdSubmitInfo.waitSemaphoreCount = 1;
	cmdSubmitInfo.pWaitSemaphores = waitSemaphores;
	cmdSubmitInfo.pWaitDstStageMask = waitStages;
	cmdSubmitInfo.commandBufferCount = 1;
	cmdSubmitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore }; // Once we're done, we'll signal these semaphores to allow the next frame to begin it's work.
	cmdSubmitInfo.signalSemaphoreCount = 1;
	cmdSubmitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(m_Device.m_GraphicsQueue, 1, &cmdSubmitInfo, m_InFlightFence)); // Signal the in-flight-fence to let us know that we're done with this cmd buffer.

	// Present our work.
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain.m_Handle;
	presentInfo.pImageIndices = &m_ImageIndex;
	presentInfo.pResults = nullptr;
	VkResult imagePresentResult = vkQueuePresentKHR(m_Device.m_PresentQueue, &presentInfo);

	// Make sure window resizes don't break anything post-presentation.
	if (imagePresentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(m_Device.m_Device);
		// We need to recreate our swapchain.
		m_Swapchain.RecreateSwapchain(s_VulkanContext, m_Swapchain);

		m_bWindowResized = false;
		return;
	}
	else if (imagePresentResult != VK_SUCCESS && imagePresentResult != VK_SUBOPTIMAL_KHR)
	{
		// If we get in here it is very likely that we won't be able to recover...
		CAssert(0);
		return;
	}

	m_bCurrentlyRendering[m_ImageIndex] = false;
}

//-------------------------------------------------------------------------
// Purpose: Draws geometry. This must happen while a frame is being rendered!
// Input: pGeometry - The geometry to be rendered.
//		  pMaterial - The material to be rendered with the geometry.
// Output: none.
//-------------------------------------------------------------------------
void CVulkanRenderer::DrawGeometry(GeometryInstance_t& pGeometry, MaterialReference& pMaterial)
{
	if (m_bResizingFramebuffer)
	{
		return;
	}

	CAssert(m_bCurrentlyRendering[m_ImageIndex]);

	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	// Bind our geometry data.
	VkDeviceSize offsets[] = { pGeometry.vertexDataOffset, pGeometry.indexDataOffset };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_GeometryVertexBuffer.m_Handle, offsets);

	// Bind the index buffer, if necessary
	if (pGeometry.indexDataOffset != 0)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_GeometryIndexBuffer.m_Handle, offsets[1], VK_INDEX_TYPE_UINT32);
	}

	// Bind our pipeline.
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultShader.m_Pipeline);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInternalShaderData[pMaterial.m_hShaderHandle].m_Pipeline);
	m_CurrentlyBoundShaderIndex = pMaterial.m_hShaderHandle;// TODO: Make this work with a call to 'BindShader()'

	int descriptorIndex = (pGeometry.hMaterial.m_iInternalHandle) ? pMaterial.m_iInternalHandle : 1; // TODO: Save off a handle to the default texture. Assuming that it will be at index 1 is bad!
	
	// Instance data not needed for now
	/* Test instance descriptor.
	{
		if (descriptorIndex == 6)
		{
			double time = (M_SIN(Platform::GetAbsoluteTime()) / 2.0) + 0.5;
			InstanceUniformObject_t instanceData;
			Vector4D colorVec{};
			colorVec.y = time;
			m_InstanceUniformData[descriptorIndex].Reserved0 = colorVec;

			void* pUniformData;
			VkDeviceSize uniformOffset = sizeof(GlobalUniformObject_t) + (sizeof(InstanceUniformObject_t) * (descriptorIndex));
			vkMapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory, uniformOffset, sizeof(InstanceUniformObject_t), 0, &pUniformData);
			memcpy(pUniformData, &m_InstanceUniformData[descriptorIndex], sizeof(InstanceUniformObject_t));
			vkUnmapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory);
		}
	}*/

	// Bind the global descriptor set.
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInternalShaderData[m_CurrentlyBoundShaderIndex].m_PipelineLayout, 0, 1, &m_pInternalShaderData[m_CurrentlyBoundShaderIndex].m_DescriptorSets[0].m_DescriptorSets[m_ImageIndex], 0, nullptr);

	// Bind instance descriptor set.
	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultShader.m_PipelineLayout, 1, 1, &m_DefaultShader.m_DescriptorSets[descriptorIndex].m_DescriptorSets[m_ImageIndex], 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInternalShaderData[m_CurrentlyBoundShaderIndex].m_PipelineLayout, 1, 1, &m_pInternalShaderData[m_CurrentlyBoundShaderIndex].m_DescriptorSets[descriptorIndex].m_DescriptorSets[m_ImageIndex], 0, nullptr);

	// Update our matrices.
	Matrix4x4 model{};
	model = Matrix4x4(1.0f);
	model = Matrix4x4::Scale(model, Vector3D(0.5f, 0.5f, 0.5f));
	//model = Matrix4x4::Rotate(model, M_DEG2RAD_F((float)m_RendererClock.GetElapsedTime()) * 60.0f, Vector3D(0, 1, 0));
	//model = Matrix4x4::Translate(model, Vector3D(0.0f, 0.0f, -150.0f));

	// Push our model matrix as well.
	vkCmdPushConstants(commandBuffer, m_pInternalShaderData[m_CurrentlyBoundShaderIndex].m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4x4), &model);

	// Issue the draw.
	if (pGeometry.indexDataOffset != 0)
	{
		vkCmdDrawIndexed(commandBuffer, pGeometry.indexDataSize, 1, pGeometry.indexDataOffset, pGeometry.vertexDataOffset, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, pGeometry.vertexCount, 1, 0, 0);
	}
}

//-------------------------------------------------------------------------
// Purpose: Acquire Vulkan specific texture resources.
// Input: pTexture		 - The texture to acquire resources for.
//		  internalIndex  - The array index/handle this texture is using.
//		  nPixelDataSize - The size (in bytes) of the texture's pixel data.
//		  pPixelData	 - A byte array containing the raw pixel data.
// Output: bool - Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::AcquireTextureResources(Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	// Allocate space for Vulkan texture data.
	VulkanTextureInternal* pInternalData = nullptr;

	// We're gonna use the same index as the one on the frontend.
	pInternalData = &m_pInternalTextureData[internalIndex];
	CAssert(pInternalData->m_Generation == kInvalidID_U8 && pInternalData->m_Image.m_Image == VK_NULL_HANDLE); // This image must not be in use (or else we most likely have a hash collision)

	if (!pInternalData)
	{
		LOG_ERROR("No more free texture slots in Vulkan backend! Aborting texture creation!");
		return false;
	}

	if (!VulkanImage::CreateImage(s_VulkanContext, pTexture.GetWidth(), pTexture.GetHeight(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, pInternalData->m_Image))
	{
		LOG_ERROR("Failed to create Vulkan resources for texture. Nothing was done.");
		delete pInternalData;
		return false;
	}

	if (!pInternalData->m_Image.WriteDataToImage(s_VulkanContext, pTexture.GetWidth(), pTexture.GetHeight(), pPixelData))
	{
		LOG_ERROR("Failed to write pixel data to texture. Nothing was done.");
		VulkanImage::DestroyImage(s_VulkanContext, pInternalData->m_Image);
		delete pInternalData;
		return false;
	}

	// If we got here, the image should be loaded and ready for use. Mark it as such...
	pInternalData->m_Generation = 0;

	// Hook us up to the frontend.
	pTexture.SetInternalTextureData(pInternalData);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases Vulkan specific texture resources.
// Input: pTexture - The texture to release resources for.
// Output: none.
//-------------------------------------------------------------------------
void CVulkanRenderer::ReleaseTextureResources(Texture& pTexture)
{
	// Make sure the resource isn't in use.
	vkDeviceWaitIdle(m_Device.m_Device);

	// Get at the Vulkan specific data.
	VulkanTextureInternal* pInternalData = reinterpret_cast<VulkanTextureInternal*>(pTexture.GetInternalData());

	if (!pInternalData)
	{
		LOG_ERROR("Failed to get Vulkan data when freeing texture '%s'. Texture release failed.", pTexture.GetName());
		return;
	}

	// Make sure we do not render anymore...
	pInternalData->m_Generation = kInvalidID_U8;

	// Release our image resources.
	VulkanImage::DestroyImage(s_VulkanContext, pInternalData->m_Image);
}

bool CVulkanRenderer::AcquireMaterialResources(Material& pMaterial, MaterialInstanceData_t& pMaterialInstanceData)
{
	// Get at our shader.
	// TODO: What to do if this shader's resources have not been loaded yet.
	// (We probably shouldn't get in here if that's the case)
	CAssert(pMaterial.m_hShaderHandle != kInvalidID_U32);
	VulkanShaderInternal* pShader = &m_pInternalShaderData[pMaterial.m_hShaderHandle];
	if (!pShader)
	{
		CAssert(0); // TODO: Handle this.
		return false;
	}

	// Get the instance descriptor set layout.
	VkDescriptorSetLayout instanceDescriptorLayout{};
	for (int idx = 0; idx < pShader->m_DescriptorSetLayouts.Capacity(); idx++)
	{
		if (pShader->m_DescriptorSetLayouts[idx].m_FrequencyLevel == eShaderUniformUpdateFrequency::OBJECT)
		{
			instanceDescriptorLayout = pShader->m_DescriptorSetLayouts[idx].m_Layout;
			break;
		}
	}
	Utl::CVector<VkDescriptorSetLayout> instanceLayouts(m_MaxImagesInFlight);
	for (int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		instanceLayouts[idx] = instanceDescriptorLayout;
	}

	// Allocate object descriptor sets.
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)m_MaxImagesInFlight;
	allocInfo.pSetLayouts = instanceLayouts.Data();
	VK_CHECK(vkAllocateDescriptorSets(m_Device.m_Device, &allocInfo, pShader->m_DescriptorSets[pShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets));

	// Acquire descriptor resources.
	for (int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
	{
		VkWriteDescriptorSet descriptorWrites[1]{};

		// First descriptor is the uniform buffer.
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = m_UniformBuffers[frameIdx].m_Handle;
		uniformBufferInfo.offset = sizeof(GlobalUniformObject_t) + (sizeof(MaterialInstanceData_t) * (pShader->m_NextFreeDescriptorSetIndex - 1)); // NOTE: we add the global UBO size here because the global uniforms live at the start of the uniform memory block.
		uniformBufferInfo.range = sizeof(MaterialInstanceData_t);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = pShader->m_DescriptorSets[pShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(m_Device.m_Device, 1, descriptorWrites, 0, nullptr);

		// Copy over the material instance data.
		void* pUniformData;
		VkDeviceSize uniformOffset = sizeof(GlobalUniformObject_t) + (sizeof(MaterialInstanceData_t) * (pShader->m_NextFreeDescriptorSetIndex - 1));
		vkMapMemory(m_Device.m_Device, m_UniformBuffers[frameIdx].m_Memory, uniformOffset, sizeof(MaterialInstanceData_t), 0, &pUniformData);
		memcpy(pUniformData, &pMaterialInstanceData, sizeof(MaterialInstanceData_t));
		vkUnmapMemory(m_Device.m_Device, m_UniformBuffers[frameIdx].m_Memory);
	}

	// TODO: Make this configurable
	// Diffuse descriptors
	VkWriteDescriptorSet imageWrites[3]{};
	for (int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pInternalTextureData[pMaterial.m_hDiffuseTextureHandle].m_Image.m_View;
		imageInfo.sampler = m_pInternalTextureData[pMaterial.m_hDiffuseTextureHandle].m_Image.m_Sampler;

		imageWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWrites[0].dstSet = pShader->m_DescriptorSets[pShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
		imageWrites[0].dstBinding = 1;
		imageWrites[0].dstArrayElement = 0;
		imageWrites[0].descriptorCount = 1;
		imageWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device.m_Device, 1, &imageWrites[0], 0, nullptr);
	}

	// Specular descriptors
	for (int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pInternalTextureData[pMaterial.m_hSpecularTextureHandle].m_Image.m_View;
		imageInfo.sampler = m_pInternalTextureData[pMaterial.m_hSpecularTextureHandle].m_Image.m_Sampler;

		imageWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWrites[1].dstSet = pShader->m_DescriptorSets[pShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
		imageWrites[1].dstBinding = 1;
		imageWrites[1].dstArrayElement = 1;
		imageWrites[1].descriptorCount = 1;
		imageWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device.m_Device, 1, &imageWrites[1], 0, nullptr);
	}

	// Normal descriptors
	for (int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pInternalTextureData[pMaterial.m_hNormalTextureHandle].m_Image.m_View;
		imageInfo.sampler = m_pInternalTextureData[pMaterial.m_hNormalTextureHandle].m_Image.m_Sampler;

		imageWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWrites[2].dstSet = pShader->m_DescriptorSets[pShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
		imageWrites[2].dstBinding = 1;
		imageWrites[2].dstArrayElement = 2;
		imageWrites[2].descriptorCount = 1;
		imageWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrites[2].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device.m_Device, 1, &imageWrites[2], 0, nullptr);
	}

	pMaterial.m_iInternalHandle = pShader->m_NextFreeDescriptorSetIndex;
	// Bump up next free descriptor index.
	pShader->m_NextFreeDescriptorSetIndex++;

	return true;
}

static VkFormat VertexAttributeToVulkanType(VertexType_t inAttribute)
{
	switch (inAttribute)
	{
	case VertexType_t::Vector2:
		return VK_FORMAT_R32G32_SFLOAT;
	case VertexType_t::Vector3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case VertexType_t::Vector4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

static size_t GetSizeOfVertexAttribute(VertexType_t inAttribute)
{
	switch (inAttribute)
	{
	case VertexType_t::Vector2:
		return sizeof(Vector2D);
	case VertexType_t::Vector3:
		return sizeof(Vector3D);
	case VertexType_t::Vector4:
		return sizeof(Vector4D);
	case VertexType_t::Matrix4x4:
		return sizeof(Matrix4x4);
	default:
		return 0;
	}
}

static VkPrimitiveTopology GetVulkanTopologyType(unsigned int inType)
{
	switch (inType)
	{
	case LINE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	default:
	case TRIANGLE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

bool CVulkanRenderer::AcquireShaderResources(Shader& pShader, ShaderConfig_t& pShaderConfig)
{
	VulkanShaderInternal& pInternalShader = m_pInternalShaderData[pShader.m_iInternalHandle];
	CAssert(pShader.m_iInternalHandle != kInvalidID_U32);

	// Acquire our shader modules.
	Utl::CVector<VkShaderModule> shaderModules(pShaderConfig.numShaderStages);
	Utl::CVector<VkShaderStageFlagBits> shaderStageTypes(pShaderConfig.numShaderStages);

	for (int idx = 0; idx < pShaderConfig.numShaderStages; idx++)
	{
		// Get the stage type.
		VkShaderStageFlagBits stage;
		{
			size_t dotIndex = StringIndexOf(pShaderConfig.pszShaderStageFileNames[idx], '.');
			char* stageName;
			stageName = StringDuplicate_N(pShaderConfig.pszShaderStageFileNames[idx] + dotIndex + 1, 4);

			if (StringCompare(stageName, "vert"))
				stage = VK_SHADER_STAGE_VERTEX_BIT;
			else if (StringCompare(stageName, "frag"))
				stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			else
			{
				LOG_ERROR("Failed to parse shader stage '%s' in shader '%s'.", stageName, pShaderConfig.pszShaderName);
				return false;
			}

			StringFree(stageName);
			shaderStageTypes[idx] = stage;
		}

		// Read in the stage bytecode.
		size_t		shaderFileSize{ 0 };
		Utl::CVector<byte> shaderBytecode{};
		{
			char formattedFilePath[kScratchStringBufferLength];
			StringFormat_V("%s/%s", formattedFilePath, "Assets/Shaders", pShaderConfig.pszShaderStageFileNames[idx]);

			FileHandle_t shaderFilehandle{};
			size_t bytesRead{ 0 };
			if (!FileSystem::OpenFile(formattedFilePath, FILEMODE_READ, true, &shaderFilehandle))
			{
				LOG_ERROR("Failed to open shader file '%s'. Shader creation aborted.", formattedFilePath);
				// TODO: Release resources.
				return false;
			}
			shaderFileSize = FileSystem::FileSize(&shaderFilehandle);
			shaderBytecode.Resize(shaderFileSize);
			
			FileSystem::ReadWholeBinaryFile(shaderBytecode.Data(), shaderFileSize, &bytesRead, &shaderFilehandle);

			if (shaderFileSize != bytesRead)
			{
				LOG_WARN("Size mismatch on shader file read: '%s'.", pShaderConfig.pszShaderStageFileNames[idx]);
			}

			FileSystem::CloseFile(&shaderFilehandle);
		}

		// Piece together the module.
		{
			VkShaderModuleCreateInfo moduleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
			moduleInfo.pNext = nullptr;
			moduleInfo.flags = 0;
			moduleInfo.codeSize = shaderFileSize;
			moduleInfo.pCode = (uint32_t*)shaderBytecode.Data();

			VK_CHECK(vkCreateShaderModule(m_Device.m_Device, &moduleInfo, m_Allocator, &shaderModules[idx]));
		}
	}

	// Create the shader's default pipeline object.
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shader stages
	Utl::CVector<VkPipelineShaderStageCreateInfo> shaderStages(pShaderConfig.numShaderStages);
	for (int stage = 0; stage < pShaderConfig.numShaderStages; stage++)
	{
		shaderStages[stage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[stage].pNext = 0;
		shaderStages[stage].flags = 0;
		shaderStages[stage].stage = shaderStageTypes[stage];
		shaderStages[stage].module = shaderModules[stage];
		shaderStages[stage].pName = "main";
		shaderStages[stage].pSpecializationInfo = nullptr;
	}

	// Vertex input state
	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkVertexInputBindingDescription vertexBindingDescription{};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex3D);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Utl::CVector<VkVertexInputAttributeDescription> attributes(pShaderConfig.vertexAttributes.Count());
	size_t currentAttributeOffset{ 0 };

	// Vertex attributes.
	for (int attrib = 0; attrib < pShaderConfig.vertexAttributes.Count(); attrib++)
	{
		attributes[attrib].binding = 0;
		attributes[attrib].location = attrib;
		attributes[attrib].format = VertexAttributeToVulkanType(pShaderConfig.vertexAttributes[attrib].type);
		attributes[attrib].offset = currentAttributeOffset;

		currentAttributeOffset += GetSizeOfVertexAttribute(pShaderConfig.vertexAttributes[attrib].type);
	}

	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = pShaderConfig.vertexAttributes.Count();
	vertexInputState.pVertexAttributeDescriptions = attributes.Data();

	// Input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = GetVulkanTopologyType(pShaderConfig.eTopologyTypes); // TODO: This only supports one type for now.
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationState{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_Swapchain.m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_Swapchain.m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain.m_SwapchainExtent;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.pNext = nullptr;
	rasterizer.flags = 0;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // No multisampling for now.

	// Depth/Stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencil.pNext = nullptr;
	depthStencil.flags = 0;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blend state
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlend.pNext = nullptr;
	colorBlend.flags = 0;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.attachmentCount = 1;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &colorBlendAttachment;
	colorBlend.blendConstants[0] = 0.0f;
	colorBlend.blendConstants[1] = 0.0f;
	colorBlend.blendConstants[2] = 0.0f;
	colorBlend.blendConstants[3] = 0.0f;

	// Dynamic pipeline state
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicState;

	// Descriptor set layout
	
	// Check for global descriptors
	bool bGlobalUniformsPresent = false;
	int	 iGlobalUniformCount = 0; // Running count of all global uniforms (NOT COUNTING SAMPLERS!)
	int  iGlobalSamplerCount = 0;

	bool bInstanceUniformsPresent = false;
	int  iInstanceUniformCount = 0; // Running count of all instance level uniforms (NOT COUNTING SAMPLERS!)
	int  iInstanceSamplerCount = 0;

	// Get the details for per-draw uniforms here.
	// They get used further down during pipeline layout creation (as they use push-constants).
	bool bPerDrawUniformsPresent = false;
	int  iPerDrawUniformCount = 0;
	int  iPerDrawMemoryRequirement = 0;

	for (int uniformIdx = 0; uniformIdx < pShaderConfig.uniformConfigs.Count(); uniformIdx++)
	{
		if (pShaderConfig.uniformConfigs[uniformIdx].eFrequency == eShaderUniformUpdateFrequency::GLOBAL)
		{
			bGlobalUniformsPresent = true;
			if (pShaderConfig.uniformConfigs[uniformIdx].eType != VertexType_t::Texture)
			{
				iGlobalUniformCount += pShaderConfig.uniformConfigs[uniformIdx].iArraySize;
			}
			else
			{
				iGlobalSamplerCount += pShaderConfig.uniformConfigs[uniformIdx].iArraySize;
			}
		}
		else if (pShaderConfig.uniformConfigs[uniformIdx].eFrequency == eShaderUniformUpdateFrequency::OBJECT)
		{
			bInstanceUniformsPresent = true;
			if (pShaderConfig.uniformConfigs[uniformIdx].eType != VertexType_t::Texture)
			{
				iInstanceUniformCount += pShaderConfig.uniformConfigs[uniformIdx].iArraySize;
			}
			else
			{
				iInstanceSamplerCount += pShaderConfig.uniformConfigs[uniformIdx].iArraySize;
			}
		}
		else if (pShaderConfig.uniformConfigs[uniformIdx].eFrequency == eShaderUniformUpdateFrequency::PER_DRAW)
		{
			CAssert(pShaderConfig.uniformConfigs[uniformIdx].eType != VertexType_t::Texture); // There may not be any per-draw samplers, as they can't use push-constants.
			bPerDrawUniformsPresent = true;
			iPerDrawUniformCount++;
			iPerDrawMemoryRequirement += GetSizeOfVertexAttribute(pShaderConfig.uniformConfigs[uniformIdx].eType);
		}
	}

	int descriptorSetLayoutCount = 0;
	if (bGlobalUniformsPresent > 0)
		descriptorSetLayoutCount++;
	if (bInstanceUniformsPresent > 0)
		descriptorSetLayoutCount++;

	pInternalShader.m_DescriptorSetLayouts.Resize(descriptorSetLayoutCount);

	int iCurrentDescriptorSetLayoutIndex = 0;
	if (bGlobalUniformsPresent)
	{
		// Set up global uniform descriptor layout.
		Utl::CVector<VkDescriptorSetLayoutBinding> globalBindings{};

		// Uniform buffer
		if (iGlobalUniformCount > 0)
		{
			VkDescriptorSetLayoutBinding globalUBOLayoutBinding{};
			globalUBOLayoutBinding.binding = 0; // Uniform buffers are always at binding 0.
			globalUBOLayoutBinding.descriptorCount = 1;
			globalUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			globalUBOLayoutBinding.pImmutableSamplers = nullptr;
			globalUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			globalBindings.PushBack(globalUBOLayoutBinding);
		}

		// Global texture samplers
		if (iGlobalSamplerCount > 0)
		{
			VkDescriptorSetLayoutBinding globalSamplerLayoutBinding{};
			globalSamplerLayoutBinding.binding = 1; // Samplers are always at binding 1.
			globalSamplerLayoutBinding.descriptorCount = iGlobalSamplerCount;
			globalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			globalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // For now at least, samplers are only used in fragment shaders.

			globalBindings.PushBack(globalSamplerLayoutBinding);
		}

		// Global descriptor set layout.
		VkDescriptorSetLayoutCreateInfo globalLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		globalLayoutInfo.bindingCount = globalBindings.Count();
		globalLayoutInfo.pBindings = globalBindings.Data();

		// TODO: Allocation callbacks.
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device.m_Device, &globalLayoutInfo, nullptr, &pInternalShader.m_DescriptorSetLayouts[iCurrentDescriptorSetLayoutIndex].m_Layout));
		pInternalShader.m_DescriptorSetLayouts[iCurrentDescriptorSetLayoutIndex].m_FrequencyLevel = eShaderUniformUpdateFrequency::GLOBAL;

		iCurrentDescriptorSetLayoutIndex++;
	}

	if (bInstanceUniformsPresent)
	{
		// Set up the instance descriptor layout.
		Utl::CVector<VkDescriptorSetLayoutBinding> instanceBindings{};

		// Instance uniform buffer.
		if (iInstanceUniformCount > 0)
		{
			VkDescriptorSetLayoutBinding instanceUBOLayoutBinding{};
			instanceUBOLayoutBinding.binding = 0; // Uniform buffers are always at binding 0.
			instanceUBOLayoutBinding.descriptorCount = 1;
			instanceUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			instanceUBOLayoutBinding.pImmutableSamplers = nullptr;
			instanceUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			instanceBindings.PushBack(instanceUBOLayoutBinding);
		}

		// Instance texture samplers.
		if (iInstanceSamplerCount > 0)
		{
			VkDescriptorSetLayoutBinding instanceSamplerLayoutBinding{};
			instanceSamplerLayoutBinding.binding = 1; // Samplers are always at binding 1.
			instanceSamplerLayoutBinding.descriptorCount = iInstanceSamplerCount;
			instanceSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			instanceSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // For now at least, samplers are only used in fragment shaders.

			instanceBindings.PushBack(instanceSamplerLayoutBinding);
		}

		// Instance descriptor set layout.
		VkDescriptorSetLayoutCreateInfo instanceLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		instanceLayoutInfo.bindingCount = instanceBindings.Count();
		instanceLayoutInfo.pBindings = instanceBindings.Data();

		// TODO: Allocation callbacks.
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device.m_Device, &instanceLayoutInfo, nullptr, &pInternalShader.m_DescriptorSetLayouts[iCurrentDescriptorSetLayoutIndex].m_Layout));
		pInternalShader.m_DescriptorSetLayouts[iCurrentDescriptorSetLayoutIndex].m_FrequencyLevel = eShaderUniformUpdateFrequency::OBJECT;

		iCurrentDescriptorSetLayoutIndex++;
	}

	// Pipeline layout
	Utl::CVector<VkDescriptorSetLayout> pDescriptorSetLayouts(descriptorSetLayoutCount);
	for (int idx = 0; idx < descriptorSetLayoutCount; idx++)
		pDescriptorSetLayouts[idx] = pInternalShader.m_DescriptorSetLayouts[idx].m_Layout;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayoutCount;
	pipelineLayoutInfo.pSetLayouts = pDescriptorSetLayouts.Data();

	// Set up push constants for per-draw uniforms.
	if (bPerDrawUniformsPresent)
	{
		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = iPerDrawMemoryRequirement;
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	}
	else
	{
		pipelineLayoutInfo.pPushConstantRanges = 0;
	}

	VK_CHECK(vkCreatePipelineLayout(m_Device.m_Device, &pipelineLayoutInfo, m_Allocator, &pInternalShader.m_PipelineLayout));

	// Create the pipeline.
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = shaderStages.Capacity();
	pipelineInfo.pStages = shaderStages.Data();
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pTessellationState = &tessellationState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = pInternalShader.m_PipelineLayout;
	pipelineInfo.renderPass = m_MainRenderPass.m_Handle; // Just using the singular renderpass for now.
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;

	VK_CHECK(vkCreateGraphicsPipelines(m_Device.m_Device, nullptr, 1, &pipelineInfo, m_Allocator, &pInternalShader.m_Pipeline));

	// Release the shader modules, as they are no longer needed.
	for (int idx = 0; idx < pShaderConfig.numShaderStages; idx++)
	{
		vkDestroyShaderModule(m_Device.m_Device, shaderModules[idx], m_Allocator);
	}

	// Descriptor set resources.
	int globalDescriptorSetCount = m_MaxImagesInFlight;							// one global descriptor set per frame in flight.
	int objectDescriptorSetCount = m_MaxImagesInFlight * kMaxVulkanInstances;	// one object descriptor set per frame in flight, enough for kMaxVulkanInstances.
	pInternalShader.m_DescriptorSets.Resize(globalDescriptorSetCount + objectDescriptorSetCount);

	// We allocate our global descriptor sets now
	// Object-level descriptor sets get allocated at shader resource acquisition.
	if (bGlobalUniformsPresent)
	{
		VkDescriptorSetLayout pLayout = pInternalShader.m_DescriptorSetLayouts[0].m_Layout; // TODO: We shouldn't assume the global will always be at index 0.
		Utl::CVector<VkDescriptorSetLayout> globalDescriptorSetLayouts(m_MaxImagesInFlight);

		for (int idx = 0; idx < m_MaxImagesInFlight; idx++)
			globalDescriptorSetLayouts[idx] = pLayout;

		VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)m_MaxImagesInFlight;
		allocInfo.pSetLayouts = globalDescriptorSetLayouts.Data();

		VK_CHECK(vkAllocateDescriptorSets(m_Device.m_Device, &allocInfo, pInternalShader.m_DescriptorSets.Data()->m_DescriptorSets));

		// Configure our new descriptors.
		for (int idx = 0; idx < m_MaxImagesInFlight; idx++)
		{
			// TODO: This should be more configurable.
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[idx].m_Handle;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(GlobalUniformObject_t);

			// TODO: Do we want global textures?
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet descriptorWrites[2]{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = pInternalShader.m_DescriptorSets[0].m_DescriptorSets[idx];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = pInternalShader.m_DescriptorSets[0].m_DescriptorSets[idx];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			// NOTE: Only writing to the global uniform buffer for now.
			vkUpdateDescriptorSets(m_Device.m_Device, 1, descriptorWrites, 0, nullptr);
		}
	}

	// Bump this to 1, as that's where instance-level descriptors start...
	pInternalShader.m_NextFreeDescriptorSetIndex = 1;

	return true;
}

void CVulkanRenderer::ReleaseShaderResources(Shader& pShader)
{
	CAssert(pShader.m_iInternalHandle != kInvalidID_U32);
	VulkanShaderInternal& pInternalShader = m_pInternalShaderData[pShader.m_iInternalHandle];
	
	vkDeviceWaitIdle(m_Device.m_Device);
	// TODO: Allocation callbacks
	if (pInternalShader.m_Pipeline)
		vkDestroyPipeline(m_Device.m_Device, pInternalShader.m_Pipeline, nullptr);

	if (pInternalShader.m_PipelineLayout)
		vkDestroyPipelineLayout(m_Device.m_Device, pInternalShader.m_PipelineLayout, nullptr);

	for (int idx = 0; idx < pInternalShader.m_DescriptorSetLayouts.Capacity(); idx++)
	{
		if (pInternalShader.m_DescriptorSetLayouts[idx].m_Layout)
		{
			vkDestroyDescriptorSetLayout(m_Device.m_Device, pInternalShader.m_DescriptorSetLayouts[idx].m_Layout, nullptr);
		}
	}
}

void CVulkanRenderer::BindShader(Shader& pShader)
{
	// TODO: Assert that a frame is being rendered.
	VulkanShaderInternal& pInternalShader = m_pInternalShaderData[pShader.m_iInternalHandle];
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pInternalShader.m_Pipeline);
	m_CurrentlyBoundShaderIndex = pShader.m_iInternalHandle;
}

bool CVulkanRenderer::AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	pOutInstance.vertexDataOffset = m_GeometryVertexBuffer.m_BytesAllocated;
	pOutInstance.vertexCount = pGeometryConfig.Vertices.Count();
	
	pOutInstance.indexDataOffset = m_GeometryIndexBuffer.m_BytesAllocated;
	pOutInstance.indexDataSize = pGeometryConfig.Indices.Count();

	VulkanCommandBuffer cmdBuffer{};
	cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Create a staging buffer for transfer to the GPU.
	VulkanBuffer stagingBuffer{};
	VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

	stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data());

	// Copy over to the device-local buffer.
	VkBufferCopy copyRegion{};
	copyRegion.size = stagingBuffer.m_BufferSize;
	vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, m_GeometryVertexBuffer.m_Handle, 1, &copyRegion);

	cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);

	// TODO: Handle index buffers as well...
	if (pGeometryConfig.Indices.Count() != 0) // TODO: Better check could be used here.
	{
		cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

		VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
		stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), pGeometryConfig.Indices.Data());

		copyRegion.size = stagingBuffer.m_BufferSize;
		vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, m_GeometryIndexBuffer.m_Handle, 1, &copyRegion);

		cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);
		VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);
	}

	m_GeometryVertexBuffer.m_BytesAllocated += pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride();
	m_GeometryIndexBuffer.m_BytesAllocated += pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride();

	return true;
}

static bool QueryValidationLayerSupport(const Utl::CVector<const char*>& expectedLayers)
{
	LOG_DEBUG("Querying Vulkan validation layer support");

	unsigned int layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	Utl::CVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());

	for (int expectedIndex = 0; expectedIndex < expectedLayers.Count(); expectedIndex++)
	{
		bool bLayerFound = false;
		LOG_VERBOSE("Searching for '%s'...", expectedLayers[expectedIndex]);

		for (unsigned int layerIndex = 0; layerIndex < layerCount; layerIndex++)
		{
			if (StringCompare(expectedLayers[expectedIndex], availableLayers[layerIndex].layerName))
			{
				bLayerFound = true;
				LOG_VERBOSE("Found!");
				break;
			}
		}

		if (!bLayerFound)
		{
			LOG_ERROR("QueryValidationLayerSupport() -> Unable to find required layer '%s'.", expectedLayers[expectedIndex]);
			return false;
		}
	}

	return true;
}

static bool QueryInstanceExtensionSupport(const Utl::CVector<const char*>& expectedExtensions)
{
	LOG_DEBUG("Querying Vulkan instance extension support");

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	Utl::CVector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.Data());

	for (int currentExt = 0; currentExt < expectedExtensions.Count(); currentExt++)
	{
		bool bExtensionFound = false;
		LOG_VERBOSE("Searching for '%s'...", expectedExtensions[currentExt]);

		for (unsigned int availableExt = 0; availableExt < extensionCount; availableExt++)
		{
			if (StringCompare(expectedExtensions[currentExt], availableExtensions[availableExt].extensionName))
			{
				LOG_VERBOSE("Found!");
				bExtensionFound = true;
			}
		}

		if (!bExtensionFound)
		{
			// If we get out here without finding a required extension, then quit out as we cannot continue.
			LOG_ERROR("QueryInstanceExtensionSupport() -> Unable to find support for extension %s", expectedExtensions[currentExt]);
			return false;
		}
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
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

static bool RendererEvent_WindowResized(EventData_t data)
{
	s_VulkanContext.m_bWindowResized = true;
	s_VulkanContext.m_bWindowResized = true;

	// Let other listeners handle this event.
	return false;
}

static bool RendererEvent_KeyPressed(EventData_t data)
{
	if (data.c[0] == 'P')
	{
		LOG_DEBUG("Camera Pos: %.8f, %.8f, %.8f", s_MainCamera.vecPosition.x, s_MainCamera.vecPosition.y, s_MainCamera.vecPosition.z);
		LOG_DEBUG("Camera Ang: %.8f, %.8f", s_MainCamera.Yaw, s_MainCamera.Pitch);
	}
	if (data.c[0] >= 0  && data.c[0] <= 9)
	{
		s_VulkanContext.m_RenderMode = data.c[0];

		switch (data.c[0])
		{
		default:
		case 0:
			LOG_INFO("Lighting mode set to default");
			break;
		case 1:
			LOG_INFO("Lighting mode set to ambient only");
			break;
		case 2:
			LOG_INFO("Lighting mode set to diffuse only");
			break;
		case 3:
			LOG_INFO("Lighting mode set to specular only");
			break;
		case 4:
			LOG_INFO("Lighting mode set to normals");
			break;
		case 5:
			LOG_INFO("Lighting mode set to disabled normal maps");
			break;
		}
	}
	return false;
}

void CVulkanRenderer::DestroyDefaultGeometry()
{
	VulkanBuffer::DestroyBuffer(s_VulkanContext, m_GeometryIndexBuffer);
	VulkanBuffer::DestroyBuffer(s_VulkanContext, m_GeometryVertexBuffer);
}