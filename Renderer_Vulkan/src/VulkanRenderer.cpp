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
#include <Math/MathLib.h>
#include <Math/VectorTypes.h>
#include <Math/VertexTypes.h>
#include <Platform/FileSystem.h>
#include <Platform/InterfaceRegistry.h>
#include <Renderer/RendererTypes.h>
#include <Resources/GeometryGenerator.h>
#include <Resources/ResourceTypes.h>
#include <Strings/String_Utils.h>

#include <Interfaces.h>

static CVulkanRenderer s_VulkanContext;
IRendererBackend* g_pRendererBackend = &s_VulkanContext;

// Forward Declarations
static bool QueryValidationLayerSupport(const Utl::CVector<const char*>& expectedLayers);
static bool QueryInstanceExtensionSupport(const Utl::CVector<const char*>& expectedExtensions);

// TODO: Shader system.
static bool AcquireShaderResources(CVulkanRenderer& pRenderingContext, VulkanShaderInternal& pShader, VulkanTextureInternal& pTexture);
static void	ReleaseShaderResources(CVulkanRenderer& pRenderingContext, VulkanShaderInternal& pShader, VulkanTextureInternal& pTexture);

static bool RendererEvent_WindowResized(EventData_t data);
static bool RendererEvent_KeyPressed(EventData_t data);

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

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

DLL_EXPORT Utl::CVector<Platform::SystemInterface_t> ObtainInterfaces()
{
	Utl::CVector<Platform::SystemInterface_t> interfaces{};

	// For this, right now we only care about the backend pointer.
	interfaces.PushBack({ g_pRendererBackend, "Renderer_Backend" });

	return interfaces;
}

DLL_EXPORT void* ConnectInterface(Utl::CVector<Platform::SystemInterface_t>& pInterfaceState)
{
	// TODO: Grab and assign interfaces needed by this module here!
	AssignInterfaces(pInterfaceState);

	// Register for window resize event.
	if (g_pEngineEvents)
	{
		g_pEngineEvents->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);
	}

	return g_pRendererBackend;
}

DLL_EXPORT void* GetRendererBackendState()
{
	return g_pRendererBackend;
}

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

	// TODO: Custom Allocator hooks.
	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));
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
	if (!Platform::CreateVulkanSurface((*this), pConfig.pHWnd))
	{
		LOG_ERROR("Failed to create Vulkan surface. Renderer initialization failed!");
		return false;
	}

	// Device
	if (!VulkanDevice::CreateDevice((*this), m_Device))
	{
		LOG_ERROR("Failed to initialize Vulkan device. Renderer initialization failed!");
		return false;
	}

	// Graphics Command Pool
	VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // We need to be able to reset our command buffers manually for now.
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = m_Device.m_QueueFamilyIndices.GraphicsQueueFamilyIndex;

	// TODO: Custom allocator hooks.
	VK_CHECK(vkCreateCommandPool(m_Device.m_Device, &commandPoolInfo, nullptr, &m_GraphicsCommandPool));

	// Allocate Graphics Command Buffers
	m_GraphicsCommandBuffers.Resize(m_MaxImagesInFlight);
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		if (!VulkanCommandBuffer::CreateCommandBuffer((*this), true, m_GraphicsCommandBuffers[idx]))
		{
			LOG_ERROR("Failed to create Vulkan Command Buffer!");
			return false;
		}
	}

	// Swapchain
	if (!VulkanSwapchain::CreateSwapchain((*this), m_Swapchain))
	{
		LOG_ERROR("Failed to initialize Vulkan Swapchain. Renderer initialization failed!");
		return false;
	}

	// Main Renderpass
	if (!VulkanRenderPass::CreateRenderpass((*this), m_MainRenderPass))
	{
		LOG_ERROR("Failed to initialize main Vulkan Renderpass. Renderer initialization failed!");
		return false;
	}

	// Swapchain Framebuffers
	if (!m_Swapchain.CreateFramebuffers((*this), m_Swapchain))
	{
		LOG_ERROR("Failed to create swapchain framebuffers. Renderer initialization failed!");
		return false;
	}

	// Create synchronization primitives.
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create our fence in a 'signaled' state so the first frame won't wait on it.

	// TODO: Custom allocator hooks.
	VK_CHECK(vkCreateSemaphore(m_Device.m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore));
	VK_CHECK(vkCreateSemaphore(m_Device.m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore));
	VK_CHECK(vkCreateFence(m_Device.m_Device, &fenceInfo, nullptr, &m_InFlightFence));

	// Create Global Descriptor Pool
	VkDescriptorPoolSize poolSizes[2]{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = (uint32_t)(m_MaxImagesInFlight + (3 * kMaxVulkanInstances));		// Enough for global uniforms and max object instances.

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 10 + (1 * kMaxVulkanInstances);									// Enough for 10 global textures and kMaxVulkanInstances of materials.

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // NOTE: This is required right now as when we recreate our temp shader (which happens on window resize), we need to be able to free and realloc our descriptors.
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;
	descriptorPoolInfo.maxSets = (uint32_t)(poolSizes[0].descriptorCount + poolSizes[1].descriptorCount);

	VK_CHECK(vkCreateDescriptorPool(m_Device.m_Device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));

	// Create our default shader.
	// THIS IS TEMPORARY
	if (!CreateDefaultShader(m_DefaultShader))
	{
		LOG_ERROR("Failed to create default shader. Renderer initialization failed!");
		return false;
	}

	// Create the temp geometry buffer.
	if (!VulkanBuffer::CreateBuffer((*this), sizeof(Vertex3D) * 108, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_GeometryVertexBuffer))
	{
		LOG_ERROR("Failed to create geometry vertex data buffer.");
		return false;
	}

	if (!VulkanBuffer::CreateBuffer((*this), sizeof(uint32_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_GeometryIndexBuffer))
	{
		LOG_ERROR("Failed to create geometry index data buffer.");
		return false;
	}

	// Allocate resources for our image data.
	m_pInternalTextureData = (VulkanTextureInternal*)g_pMemoryManager->Allocate(sizeof(VulkanTextureInternal) * pConfig.MaxTextures);

	// Invalidate all entries.
	for (int idx = 0; idx < pConfig.MaxTextures; idx++)
	{
		m_pInternalTextureData[idx].m_Generation = kInvalidID_U8;
	}

	// Save off our configuration.
	m_RendererConfig = pConfig;

	// Register for input events.
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);

	s_MainCamera.RecalculateCameraVectors();

	m_RendererClock.Start();

	return true;
}

void CVulkanRenderer::RendererBackend_Shutdown()
{
	// TODO: Custom Allocator hooks.

	// Make sure none of our resources are in use.
	vkDeviceWaitIdle(m_Device.m_Device);

	// Destroy our temp geometry.
	// THIS IS TEMPORARY
	DestroyDefaultGeometry();

	// Destroy our default shader.
	// THIS IS TEMPORARY
	DestroyDefaultShader(m_DefaultShader);

	// Free internal texture data.
	g_pMemoryManager->Free(m_pInternalTextureData);

	// Descriptor Pool
	vkDestroyDescriptorPool(m_Device.m_Device, m_DescriptorPool, nullptr);

	// Sync objects.
	vkDestroyFence(m_Device.m_Device, m_InFlightFence, nullptr);
	vkDestroySemaphore(m_Device.m_Device, m_RenderFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_Device.m_Device, m_ImageAvailableSemaphore, nullptr);

	// Main Renderpass
	VulkanRenderPass::DestroyRenderpass((*this), m_MainRenderPass);

	// Swapchain
	VulkanSwapchain::DestroySwapchain((*this), m_Swapchain);

	// Graphics Command Pool
	vkDestroyCommandPool(m_Device.m_Device, m_GraphicsCommandPool, nullptr);

	// Device
	VulkanDevice::DestroyDevice((*this), m_Device);

#ifdef _DEBUG
	// Debug messenger
	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebuggerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

	if (destroyDebuggerFunc != nullptr)
	{
		destroyDebuggerFunc(m_Instance, m_DebugMessenger, nullptr);
		return;
	}
#endif

	// Instance
	vkDestroyInstance(m_Instance, nullptr);
}

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

	if (imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(m_Device.m_Device);
		// We need to recreate our swapchain.
		m_Swapchain.RecreateSwapchain((*this), m_Swapchain);

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

	// Update our matrices.
	Matrix4x4 model{};
	model = Matrix4x4(1.0f);
	model = Matrix4x4::Rotate(model, M_DEG2RAD_F((float)m_RendererClock.GetElapsedTime()) * 60.0f, Vector3D(0, 1, 0));
	model = Matrix4x4::Translate(model, Vector3D(0.0f, 0.0f, -100.0f));

	m_UniformData.View = s_MainCamera.GetViewMatrix();
	m_UniformData.Projection = Matrix4x4::Perspective(M_DEG2RAD_F(45.0f), m_Swapchain.m_SwapchainExtent.width / (float)m_Swapchain.m_SwapchainExtent.height, 0.1f, 1000.0f);
	m_UniformData.Projection.data[5] *= -1;

	// Copy to the GPU.
	void* pUniformData;
	VkDeviceSize uniformOffset = { 0 };
	vkMapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory, uniformOffset, sizeof(GlobalUniformObject_t), 0, &pUniformData);
	memcpy(pUniformData, &m_UniformData, sizeof(GlobalUniformObject_t));
	vkUnmapMemory(m_Device.m_Device, m_UniformBuffers[m_ImageIndex].m_Memory);

	// Push our model matrix as well.
	vkCmdPushConstants(commandBuffer, m_DefaultShader.m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4x4), &model);

	return true;
}

void CVulkanRenderer::EndFrame(RenderPacket_t& pRenderData)
{
	if (m_bResizingFramebuffer)
	{
		return;
	}

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
	// TODO: Instance descriptor sets are still borked after resize. This is most likely to to do with destroying and recreating our shader, 
	// we need to re-evaluate this after we setup dynamic rendering.
	if (imagePresentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(m_Device.m_Device);
		// We need to recreate our swapchain.
		m_Swapchain.RecreateSwapchain((*this), m_Swapchain);

		m_bWindowResized = false;
		return;
	}
	else if (imagePresentResult != VK_SUCCESS && imagePresentResult != VK_SUBOPTIMAL_KHR)
	{
		// If we get in here it is very likely that we won't be able to recover...
		CAssert(0);
		return;
	}
}

void CVulkanRenderer::DrawGeometry(GeometryInstance_t& pGeometry, Texture& pTexture)
{
	if (m_bResizingFramebuffer)
	{
		return;
	}

	// TODO: Check to make sure we're in a renderpass first!
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	// Bind our geometry data.
	VkDeviceSize offsets[] = { pGeometry.vertexDataOffset };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_GeometryVertexBuffer.m_Handle, offsets);
	// TODO: Index buffer

	// Bind our pipeline.
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultShader.m_Pipeline);

	// Bind instance descriptor set.
	int descriptorIndex = (pGeometry.pTexture) ? reinterpret_cast<VulkanTextureInternal*>(pTexture.GetInternalData())->m_DescriptorSetIndex : 1; // TODO: Save off a handle to the default texture. Assuming that it will be at index 1 is bad!
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DefaultShader.m_PipelineLayout, 0, 1, &m_DefaultShader.m_DescriptorSets[descriptorIndex].m_DescriptorSets[m_ImageIndex], 0, nullptr);

	// Issue the draw.
	vkCmdDraw(commandBuffer, pGeometry.vertexCount, 1, 0, 0);
}

bool CVulkanRenderer::AcquireTextureResources(Texture& pTexture, size_t nPixelDataSize, byte* pPixelData)
{
	// Allocate space for Vulkan texture data.
	VulkanTextureInternal* pInternalData = nullptr;

	// Look for a free texture slot.
	for (int idx = 0; idx < m_RendererConfig.MaxTextures; idx++)
	{
		if (m_pInternalTextureData[idx].m_Generation == kInvalidID_U8)
		{
			// Found one!
			pInternalData = &m_pInternalTextureData[idx];
			break;
		}
	}

	if (!pInternalData)
	{
		LOG_ERROR("No more free texture slots in Vulkan backend! Aborting texture creation!");
		return false;
	}

	if (!VulkanImage::CreateImage((*this), pTexture.GetWidth(), pTexture.GetHeight(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, pInternalData->m_Image))
	{
		LOG_ERROR("Failed to create Vulkan resources for texture. Nothing was done.");
		delete pInternalData;
		return false;
	}

	if (!pInternalData->m_Image.WriteDataToImage((*this), pTexture.GetWidth(), pTexture.GetHeight(), pPixelData))
	{
		LOG_ERROR("Failed to write pixel data to texture. Nothing was done.");
		VulkanImage::DestroyImage((*this), pInternalData->m_Image);
		delete pInternalData;
		return false;
	}

	// Acquire shader resources.
	if (!AcquireShaderResources((*this), m_DefaultShader, *pInternalData))
	{
		LOG_ERROR("Failed to acquire shader resources for test texture!");
		VulkanImage::DestroyImage((*this), pInternalData->m_Image);
		delete pInternalData;
		return false;
	}

	// If we got here, the image should be loaded and ready for use. Mark it as such...
	pInternalData->m_Generation = 0;

	// Hook us up to the frontend.
	pTexture.SetInternalTextureData(pInternalData);

	return true;
}

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

	// Release shader resources.
	ReleaseShaderResources((*this), m_DefaultShader, *pInternalData);

	// Release our image resources.
	VulkanImage::DestroyImage((*this), pInternalData->m_Image);
}

bool CVulkanRenderer::AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	pOutInstance.vertexDataOffset = m_GeometryVertexBuffer.m_BytesAllocated;
	pOutInstance.vertexCount = pGeometryConfig.Vertices.Capacity();
	// TODO: Handle index buffer.
	m_GeometryVertexBuffer.UploadDataRange((*this), pGeometryConfig.Vertices.Capacity() * pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data());

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

static bool AcquireShaderResources(CVulkanRenderer& pRenderingContext, VulkanShaderInternal& pShader, VulkanTextureInternal& pTexture)
{
	VkDescriptorSetLayout layouts[3] = { pShader.m_DescriptorSetLayouts, pShader.m_DescriptorSetLayouts, pShader.m_DescriptorSetLayouts };

	// Allocate object descriptor sets.
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = pRenderingContext.m_DescriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)pRenderingContext.m_MaxImagesInFlight;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK(vkAllocateDescriptorSets(pRenderingContext.m_Device.m_Device, &allocInfo, pShader.m_DescriptorSets[pShader.m_NextFreeDescriptorSetIndex].m_DescriptorSets));

	// Configure our new descriptors.
	for (unsigned int idx = 0; idx < pRenderingContext.m_MaxImagesInFlight; idx++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = pRenderingContext.m_UniformBuffers[idx].m_Handle;
		bufferInfo.offset = (sizeof(InstanceUniformObject_t) * pTexture.m_DescriptorSetIndex);
		bufferInfo.range = sizeof(InstanceUniformObject_t);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = pTexture.m_Image.m_View;
		imageInfo.sampler = pTexture.m_Image.m_Sampler;

		VkWriteDescriptorSet descriptorWrites[2]{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = pShader.m_DescriptorSets[pShader.m_NextFreeDescriptorSetIndex].m_DescriptorSets[idx];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = pShader.m_DescriptorSets[pShader.m_NextFreeDescriptorSetIndex].m_DescriptorSets[idx];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(pRenderingContext.m_Device.m_Device, 2, descriptorWrites, 0, nullptr);
	}
	pTexture.m_DescriptorSetIndex = pShader.m_NextFreeDescriptorSetIndex;

	// Bump up the next free index...
	pShader.m_NextFreeDescriptorSetIndex++;

	return true;
}

static void	ReleaseShaderResources(CVulkanRenderer& pRenderingContext, VulkanShaderInternal& pShader, VulkanTextureInternal& pTexture)
{
	// TODO: Keep a freelist or something for freed descriptors.
	vkFreeDescriptorSets(pRenderingContext.m_Device.m_Device, pRenderingContext.m_DescriptorPool, pRenderingContext.m_MaxImagesInFlight, pShader.m_DescriptorSets[pTexture.m_DescriptorSetIndex].m_DescriptorSets);

	pTexture.m_DescriptorSetIndex = kInvalidID_U8;
}

bool CVulkanRenderer::CreateDefaultShader(VulkanShaderInternal& pOutShader)
{
	VkShaderModule vertShaderModule{ VK_NULL_HANDLE };
	VkShaderModule fragShaderModule{ VK_NULL_HANDLE };

	// Read in the vert module bytecode.
	FileHandle_t vertFileHandle{};
	if (!FileSystem::OpenFile("Assets/Shaders/vert.spv", FILEMODE_READ, true, &vertFileHandle))
	{
		LOG_ERROR("Failed to open vertex shader bytecode, shader initialization cannot continue.");
		return false;
	}

	size_t vertFileSize = FileSystem::FileSize(&vertFileHandle);
	size_t vertBytesRead = 0;
	Utl::CVector<byte> vertBytecode(vertFileSize);
	FileSystem::ReadWholeBinaryFile(vertBytecode.Data(), vertFileSize, &vertBytesRead, &vertFileHandle);

	if (vertBytesRead != vertFileSize)
	{
		LOG_WARN("Size mismatch in reading file '%s'.", vertFileHandle.path);
	}

	// Create the vertex module.
	VkShaderModuleCreateInfo vertModuleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertModuleInfo.pNext = nullptr;
	vertModuleInfo.flags = 0;
	vertModuleInfo.codeSize = vertFileSize;
	vertModuleInfo.pCode = (uint32_t*)vertBytecode.Data();

	VK_CHECK(vkCreateShaderModule(m_Device.m_Device, &vertModuleInfo, nullptr, &vertShaderModule));

	// Read in the frag module bytecode.
	FileHandle_t fragFileHandle{};
	if (!FileSystem::OpenFile("Assets/Shaders/frag.spv", FILEMODE_READ, true, &fragFileHandle))
	{
		LOG_ERROR("Failed to open fragment shader bytecode, shader initialization cannot continue.");
		return false;
	}

	size_t fragFileSize = FileSystem::FileSize(&fragFileHandle);
	size_t fragBytesRead = 0;
	Utl::CVector<byte> fragBytecode(fragFileSize);
	FileSystem::ReadWholeBinaryFile(fragBytecode.Data(), fragFileSize, &fragBytesRead, &fragFileHandle);

	if (fragBytesRead != fragFileSize)
	{
		LOG_WARN("Size mismatch in reading file '%s'.", fragFileHandle.path);
	}

	// Create the frag module.
	VkShaderModuleCreateInfo fragModuleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragModuleInfo.pNext = nullptr;
	fragModuleInfo.flags = 0;
	fragModuleInfo.codeSize = fragFileSize;
	fragModuleInfo.pCode = (uint32_t*)fragBytecode.Data();

	VK_CHECK(vkCreateShaderModule(m_Device.m_Device, &fragModuleInfo, nullptr, &fragShaderModule));

	// Create the shader's default pipeline object.
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shader stages
	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].pNext = 0;
	shaderStages[0].flags = 0;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].pNext = 0;
	shaderStages[1].flags = 0;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragShaderModule;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = nullptr;

	// Vertex input state
	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputState.pNext = nullptr;
	vertexInputState.flags = 0;

	VkVertexInputBindingDescription vertexBindingDescription{};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex3D);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescriptions[2]{};
	vertexAttributeDescriptions[0].location = 0;	// Vertex position
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;

	vertexAttributeDescriptions[1].location = 1;	// Texcoord
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[1].offset = offsetof(Vertex3D, TexCoord);

	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = 2;
	vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	// Input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.pNext = nullptr;
	inputAssemblyState.flags = 0;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	// Tessellation state
	// Not really using this tho.
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
	// Uniform buffer
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Sampler
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;
	VK_CHECK(vkCreateDescriptorSetLayout(m_Device.m_Device, &layoutInfo, nullptr, &pOutShader.m_DescriptorSetLayouts));

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &pOutShader.m_DescriptorSetLayouts;

	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(Matrix4x4);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	VK_CHECK(vkCreatePipelineLayout(m_Device.m_Device, &pipelineLayoutInfo, nullptr, &pOutShader.m_PipelineLayout));

	// Create the pipeline.
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pTessellationState = &tessellationState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = pOutShader.m_PipelineLayout;
	pipelineInfo.renderPass = m_MainRenderPass.m_Handle; // Just using the singular renderpass for now.
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;

 	VK_CHECK(vkCreateGraphicsPipelines(m_Device.m_Device, nullptr, 1, &pipelineInfo, nullptr, &m_DefaultShader.m_Pipeline));

	// The shader modules are no longer needed past this point.
	vkDestroyShaderModule(m_Device.m_Device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_Device.m_Device, vertShaderModule, nullptr);

	// Create the temp uniform buffer.
	m_UniformBuffers.Resize(m_MaxImagesInFlight);
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		if (!VulkanBuffer::CreateBuffer((*this), (sizeof(GlobalUniformObject_t) * 1) + (sizeof(InstanceUniformObject_t) * kMaxVulkanInstances), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[idx]))
		{
			LOG_ERROR("Failed to create uniform buffer.");
			return false;
		}
	}

	// Allocate descriptor sets.
	int globalDescriptorSetCount = 3;							// 3 global descriptors, one per frame in flight.
	int objectDescriptorSetCount = 3 * kMaxVulkanInstances;		// 3 object descriptor sets per frame in flight, enough for kMaxVulkanInstances.
	pOutShader.m_DescriptorSets.Resize(globalDescriptorSetCount + objectDescriptorSetCount);

	// NOTE: We are using the same descriptor set layout for globals and objects for now,
	// Binding 0 is the uniform buffer, binding 1 is an array of image samplers.
	Utl::CVector<VkDescriptorSetLayout> layouts(m_MaxImagesInFlight);
	for (int idx = 0; idx < layouts.Capacity(); idx++)
	{
		layouts.PushBack(pOutShader.m_DescriptorSetLayouts);
	}

	// We allocate our global descriptor sets now
	// Object-level descriptors get allocated at shader resource acquisition.
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)m_MaxImagesInFlight;
	allocInfo.pSetLayouts = layouts.Data();

	VK_CHECK(vkAllocateDescriptorSets(m_Device.m_Device, &allocInfo, pOutShader.m_DescriptorSets.Data()->m_DescriptorSets));

	// Configure our new descriptors.
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffers[idx].m_Handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GlobalUniformObject_t);

		// TODO: Do we want global textures?
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet descriptorWrites[2]{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = pOutShader.m_DescriptorSets[0].m_DescriptorSets[idx];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = pOutShader.m_DescriptorSets[0].m_DescriptorSets[idx];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		// NOTE: Only writing to the global uniform buffer for now.
		vkUpdateDescriptorSets(m_Device.m_Device, 1, descriptorWrites, 0, nullptr);
	}

	// Bump this to 1, as that's where instance level descriptors start...
	pOutShader.m_NextFreeDescriptorSetIndex = 1;

	return true;
}

void CVulkanRenderer::DestroyDefaultShader(VulkanShaderInternal& pShader)
{
	// We need to free our descriptors in the case that we're about to recreate our shader.
	vkFreeDescriptorSets(m_Device.m_Device, m_DescriptorPool, m_MaxImagesInFlight, pShader.m_DescriptorSets.Data()->m_DescriptorSets);
	vkDestroyDescriptorSetLayout(m_Device.m_Device, pShader.m_DescriptorSetLayouts, nullptr);

	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		VulkanBuffer::DestroyBuffer((*this), m_UniformBuffers[idx]);
	}

	vkDestroyPipeline(m_Device.m_Device, pShader.m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_Device.m_Device, pShader.m_PipelineLayout, nullptr);
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
	return false;
}

void CVulkanRenderer::DestroyDefaultGeometry()
{
	VulkanBuffer::DestroyBuffer((*this), m_GeometryIndexBuffer);
	VulkanBuffer::DestroyBuffer((*this), m_GeometryVertexBuffer);
}