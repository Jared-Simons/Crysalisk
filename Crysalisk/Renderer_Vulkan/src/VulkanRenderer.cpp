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
#include <Renderer/Camera.h>
#include <Renderer/RendererTypes.h>
#include <Resources/GeometryGenerator.h>
#include <Resources/Loaders/ShaderLoader.h>
#include <Resources/ResourceTypes.h>
#include <Strings/String_Utils.h>
#include <Systems/TextureSystem.h>

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

// Renderer event handlers.
static bool RendererEvent_WindowResized(EventData_t data);
static bool RendererEvent_KeyPressed(EventData_t data);

//-------------------------------------------------------------------------
// Purpose: Gets opaque pointers to interfaces that will be exposed to the rest of the engine.
// Input:	pOutInterfaces - An array to add our interfaces to.
// Output:	none.
//-------------------------------------------------------------------------
DLL_EXPORT void ObtainInterfaces(Utl::CVector<Platform::SystemInterface_t>& pOutInterfaces)
{
	// For this, right now we only care about the backend pointer.
	pOutInterfaces.PushBack({ g_pRendererBackend, "Renderer_Backend" });
}

//-------------------------------------------------------------------------
// Purpose: Connect interfaces provided by other libraries and do any work to make sure they're present.
// Input: pInterfaceState - An array of interfaces to be setup.
// Output: none.
//-------------------------------------------------------------------------
DLL_EXPORT void ConnectInterface(Utl::CVector<Platform::SystemInterface_t>& pInterfaceState)
{
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
// Input:	pConfig - Renderer specific configuration provided by the calling application.
// Output:	bool - Returns true on successful initialization, otherwise false.
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

#if defined _DEBUG || defined VULKAN_RENDERER_RESOURCE_DEBUGGING_ENABLED // We need the debug utils extension here as well, since this handles all resource tagging.
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

	// Find the necessary PFNs for resource-tagging.
#ifdef VULKAN_RENDERER_RESOURCE_DEBUGGING_ENABLED
	m_pfnSetDebugObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
	if (!m_pfnSetDebugObjectName)
	{
		LOG_WARN("Failed to find Vulkan function pointer 'vkSetDebugUtilsObjectNameEXT' despite having resource debugging enabled. Resources will not be tagged.");
	}
#endif

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
	if (!Platform::CreateVulkanSurface(s_VulkanContext, pConfig.pMainWindow->GetInternalHandle()))
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

	// TODO: We shouldn't need these passes anymore...
	// Main Renderpass
	if (!VulkanRenderPass::CreateRenderpass(s_VulkanContext, "World_Pass", true, m_MainRenderPass))
	{
		LOG_ERROR("Failed to initialize main Vulkan Renderpass. Renderer initialization failed!");
		return false;
	}

	// UI Renderpass
	if (!VulkanRenderPass::CreateRenderpass(s_VulkanContext, "UI_Pass", false, m_UIRenderPass))
	{
		LOG_ERROR("Failed to initialize UI Vulkan Renderpass. Renderer initialization failed!");
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

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the Vulkan renderer.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::RendererBackend_Shutdown()
{
	// Make sure none of our resources are in use.
	vkDeviceWaitIdle(m_Device.m_Device);

	// Free internal texture/shader data.
	delete[] m_pInternalShaderData;
	delete[] m_pInternalTextureData;

	// Sync objects.
	vkDestroyFence(m_Device.m_Device, m_InFlightFence, m_Allocator);
	vkDestroySemaphore(m_Device.m_Device, m_RenderFinishedSemaphore, m_Allocator);
	vkDestroySemaphore(m_Device.m_Device, m_ImageAvailableSemaphore, m_Allocator);

	// UI Renderpass
	VulkanRenderPass::DestroyRenderpass(s_VulkanContext, m_UIRenderPass);

	// Main Renderpass
	VulkanRenderPass::DestroyRenderpass(s_VulkanContext, m_MainRenderPass);

	// Swapchain
	VulkanSwapchain::DestroySwapchain(s_VulkanContext, m_Swapchain);

	// Graphics Command Pool
	vkDestroyCommandPool(m_Device.m_Device, m_GraphicsCommandPool, m_Allocator);

	// Device
	VulkanDevice::DestroyDevice(s_VulkanContext, m_Device);

	// Surface
	vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);

#ifdef _DEBUG
	// Debug messenger
	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebuggerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

	if (destroyDebuggerFunc != nullptr)
	{
		destroyDebuggerFunc(m_Instance, m_DebugMessenger, m_Allocator);
	}
#endif

	// Instance
	vkDestroyInstance(m_Instance, m_Allocator);

	// Clear out any heap allocations!
	m_GraphicsCommandBuffers.Clear();
	m_Swapchain.m_Framebuffers.Clear();
	m_Swapchain.m_SwapchainImageViews.Clear();
	m_Swapchain.m_SwapchainImages.Clear();
}

//-------------------------------------------------------------------------
// Purpose: Begins a renderer frame.
// Input:	pRenderData - Frame specific render data provided by the application.
// Output:	bool - Returns true on successful start, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::StartFrame(RenderPacket_t& pRenderData)
{
	if (m_bResizingFramebuffer)
	{
		return true;
	}

	// Wait for the prior frame to finish it's work.
	vkWaitForFences(m_Device.m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

	//s_MainCamera.UpdateCameraData(pRenderData.DeltaTime);

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
		m_bCurrentlyRendering[m_ImageIndex] = false;
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

	// Set our dynamic state.
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

	m_CurrentRendererFrame++;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Ends a renderer frame.
// Input:	pRenderData - Frame specific render data provided by the application.
// Output:	bool - Returns true on successful end, otherwise false.
//-------------------------------------------------------------------------
void CVulkanRenderer::EndFrame(RenderPacket_t& pRenderData)
{
	if (m_bResizingFramebuffer)
	{
		return;
	}

	CAssert(m_bCurrentlyRendering[m_ImageIndex]);

	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	// End and submit our command buffer.
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo cmdSubmitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore }; // We'll be waiting on this for the image to become available.
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
		m_bCurrentlyRendering[m_ImageIndex] = false;
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
// Purpose: Begins a provided renderpass.
// Input:	pRendererView - The renderpass to be started.
// Output:	Returns true on successfull pass start, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::StartRenderpass(RendererView& pRendererView)
{
	VkRenderPassBeginInfo renderpassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	VulkanRenderPass* pInternalPass = reinterpret_cast<VulkanRenderPass*>(pRendererView.m_pInternalData);
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	renderpassInfo.renderPass = pInternalPass->m_Handle;
	renderpassInfo.framebuffer = m_Swapchain.m_Framebuffers[m_ImageIndex];
	renderpassInfo.renderArea.offset = { 0, 0 };
	renderpassInfo.renderArea.extent = m_Swapchain.m_SwapchainExtent;

	VkClearValue clearColors[2] = { {0.3f, 0.0f, 0.3f, 1.0f}, {1.0f, 0} };
	renderpassInfo.clearValueCount = 2;
	renderpassInfo.pClearValues = clearColors;

	vkCmdBeginRenderPass(commandBuffer, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Ends the current renderpass.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::EndRenderpass()
{
	// TODO: Maybe store the current renderpass in a stack or something?
	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;
	vkCmdEndRenderPass(commandBuffer);
}

//-------------------------------------------------------------------------
// Purpose: Draws geometry. This must happen while a frame is being rendered!
// Input:	pVertexBuffer - The vertex buffer associated with the geometry.
//			pIndexBuffer - The index buffer associated with the geometry.
//			pGeometry - The geometry instance to be rendered.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::DrawGeometry(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry)
{
	CAssert(m_pCurrentlyBoundShader);
	VulkanBuffer* pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer* pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	if (m_bResizingFramebuffer)
	{
		return;
	}

	if (m_CurrentlyBoundShaderIndex == kInvalidID_U32)
	{
		// No shader is currently bound! Boot out.
		LOG_WARN("DrawGeometry() called with an invalid shader bound! Aborting draw.");
		return;
	}

	CAssert(m_bCurrentlyRendering[m_ImageIndex]);

	VkCommandBuffer commandBuffer = m_GraphicsCommandBuffers[m_ImageIndex].m_Handle;

	// Bind our geometry data.
	VkDeviceSize offsets[] = { pGeometry.vertexDataOffset, pGeometry.indexDataOffset };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &pInternalVertexBuffer->m_Handle, &offsets[0]);

	// Bind the index buffer, if necessary
	if (pGeometry.indexDataOffset != kInvalidID_U64)
	{
		vkCmdBindIndexBuffer(commandBuffer, pInternalIndexBuffer->m_Handle, offsets[1], VK_INDEX_TYPE_UINT32);
	}

	// Issue the draw.
	if (pGeometry.indexDataOffset != kInvalidID_U64)
	{
		//vkCmdDrawIndexed(commandBuffer, (uint32_t)pGeometry.indexDataSize, 1, (uint32_t)pGeometry.indexDataOffset, (int32_t)pGeometry.vertexDataOffset, 0);
		vkCmdDrawIndexed(commandBuffer, (uint32_t)pGeometry.indexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, (uint32_t)pGeometry.vertexCount, 1, 0, 0);
	}
}

//-------------------------------------------------------------------------
// Purpose: Converts an engine-generic buffer usage format to a Vulkan-specific format.
// Input:	usage - The engine-generic buffer usage.
// Output:	The Vulkan-specific buffer usage.
//-------------------------------------------------------------------------
static VkBufferUsageFlags ConvertGenericBufferUsageToVulkanFormat(eRenderBufferUsage usage)
{
	switch (usage)
	{
	default:
		return 0;
	case eRenderBufferUsage::VERTEX:
		return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	case eRenderBufferUsage::INDEX:
		return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	case eRenderBufferUsage::UNIFORM:
		return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	case eRenderBufferUsage::STORAGE:
		return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
}

//-------------------------------------------------------------------------
// Purpose: Creates an internal Vulkan buffer, and hooks it up to the generic buffer structure.
// Input:	bufferSize - The size in bytes of the buffer.
//			bufferUsage - Indicates the buffer's usage (i.e. Vertex, Index, Uniform).
//			bUseFreelist - Indicates if this buffer will use a freelist to track it's memory.
//			pOutBuffer - The generic buffer structure that will hold this buffer.
// Output:	Returns true on successful buffer creation, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateRenderBufferInternal(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, RenderBuffer& pOutBuffer)
{
	// Allocate our internal buffer.
	pOutBuffer.m_pInternalData = new VulkanBuffer;
	VulkanBuffer* pInternalBuffer = reinterpret_cast<VulkanBuffer*>(pOutBuffer.m_pInternalData);

	// Get our converted buffer usage flags
	VkBufferUsageFlags vkBufferUsage = ConvertGenericBufferUsageToVulkanFormat(bufferUsage);

	// Create the buffer.
	if (!VulkanBuffer::CreateBuffer(s_VulkanContext, bufferSize, vkBufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bUseFreelist, *pInternalBuffer))
	{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Destroys an internal Vulkan Buffer.
// Input:	pRenderBuffer - The buffer whose internal resources to destroy.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::DestroyRenderBufferInternal(RenderBuffer& pRenderBuffer)
{
	VulkanBuffer* pInternalBuffer = reinterpret_cast<VulkanBuffer*>(pRenderBuffer.m_pInternalData);
	VulkanBuffer::DestroyBuffer(s_VulkanContext, *pInternalBuffer);

	delete pRenderBuffer.m_pInternalData;
	pRenderBuffer.m_pInternalData = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Creates an internal Vulkan renderpass and sets it up to the generic renderer view.
// Input:	pszPassName - The name to be associated with this renderpass.
//			pOutView - The generic renderer view structure that will hold this renderpass.
// Output:	Returns true on successful renderpass creation, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateRendererViewInternal(char* pszPassName, RendererView& pOutView)
{
	// Allocate our internal renderpass.
	pOutView.m_pInternalData = new VulkanRenderPass;
	VulkanRenderPass* pInternalPass = reinterpret_cast<VulkanRenderPass*>(pOutView.m_pInternalData);

	return VulkanRenderPass::CreateRenderpass(s_VulkanContext, pszPassName, pOutView.m_bShouldClear, *pInternalPass);
}

//-------------------------------------------------------------------------
// Purpose: Destroys an internal Vulkan Renderpass.
// Input:	pView - The renderer view whose internal resources to destroy.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::DestroyRendererViewInternal(RendererView& pView)
{
	VulkanRenderPass* pInternalPass = reinterpret_cast<VulkanRenderPass*>(pView.m_pInternalData);
	VulkanRenderPass::DestroyRenderpass(s_VulkanContext, *pInternalPass);

	delete pView.m_pInternalData;
	pView.m_pInternalData = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Gets a Vulkan-specific format for a number of channels.
// Input:	iChannelCount - The channel count to get a Vulkan format for.
// Output:	The Vulkan-specific format that represents the channel count.
//-------------------------------------------------------------------------
static VkFormat ConvertChannelNumberToVulkanFormat(int iChannelCount)
{
	switch (iChannelCount)
	{
	case 1:
		return VK_FORMAT_R8_UNORM;
	case 3:
	case 4:
	default:
		if (iChannelCount == 0)
			LOG_WARN("Channel count of 0 was provided, returning R8G8B8A8.");
		return VK_FORMAT_R8G8B8A8_UNORM;
	}
}

//-------------------------------------------------------------------------
// Purpose: Acquires Vulkan-specific resources for images/textures.
// Input:	pTexture - The generic front-end texture structure.
//			internalIndex - The front-end handle that represents this texture. This should match on the backend.
//			nPixelDataSize - The size in bytes of the pixel data for this image.
//			pPixelData - The pixel array for this image.
// Output:	Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::AcquireTextureResources(Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	// The internal data for a Vulkan texture is the image data.
	pTexture->internalHandle = internalIndex;
	VulkanImage* pImageInternal = &m_pInternalTextureData[internalIndex].m_Image;

	// Set up the internal image data.
	VkFormat format = ConvertChannelNumberToVulkanFormat(pTexture->numChannels);
	if (!VulkanImage::CreateImage(s_VulkanContext, pTexture->pszTextureName, pTexture->width, pTexture->height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, *pImageInternal))
	{
		LOG_ERROR("Failed to create Vulkan resources for texture.");
		return false;
	}

	if (!pImageInternal->WriteDataToImage(s_VulkanContext, pTexture->width, pTexture->height, pTexture->numChannels, pPixelData))
	{
		LOG_ERROR("Failed to write pixel data to texture. Nothing was done.");
		VulkanImage::DestroyImage(s_VulkanContext, *pImageInternal);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases Vulkan-specific resources for images/textures.
// Input:	pTexture - The generic texture whose resources are to be released.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::ReleaseTextureResources(Texture* pTexture)
{
	// Make sure the resource isn't in use.
	vkDeviceWaitIdle(m_Device.m_Device);

	VulkanImage* pImageInternal = &m_pInternalTextureData[pTexture->internalHandle].m_Image;
	CAssert(pImageInternal);

	// Destroy the image data.
	VulkanImage::DestroyImage(s_VulkanContext, *pImageInternal);
}

//-------------------------------------------------------------------------
// Purpose: Gets a Vulkan-specific shader stage flag from a generic type.
// Input:	inStage - The generic shader stage.
// Output:	The Vulkan-specific shader stage flag.
//-------------------------------------------------------------------------
static VkShaderStageFlagBits ConvertShaderStageToVulkanFormat(eShaderStage inStage)
{
	switch (inStage)
	{
	case eShaderStage::VERTEX:
	case eShaderStage::UNDEFINED:
	default:
		return VK_SHADER_STAGE_VERTEX_BIT;

	case eShaderStage::FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
}

//-------------------------------------------------------------------------
// Purpose: Creates a Vulkan shader module from the given configuration.
// Input:	pRenderingContext - The owning Vulkan context.
//			pInternalShader - The internal shader that will own this module.
//			pConfig - The configuration for this shader module.
//			pOutModules - An array of shader modules that will hold the created module.
// Output:	Returns true on successful module creation, otherwise false.
//-------------------------------------------------------------------------
static bool CreateShaderModule(CVulkanRenderer& pRenderingContext, VulkanShaderInternal* pInternalShader, ShaderModuleConfig_t& pConfig, Utl::CVector<VulkanShaderModule_t>& pOutModules)
{
	VkShaderModuleCreateInfo shaderModuleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shaderModuleInfo.codeSize = pConfig.iCodeSize;
	shaderModuleInfo.pCode = (uint32_t*)pConfig.byteCode.Data();

	VulkanShaderModule_t newModule{};
	if (vkCreateShaderModule(pRenderingContext.m_Device.m_Device, &shaderModuleInfo, pRenderingContext.m_Allocator, &newModule.hModule) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create shader module.");
		return false;
	}
	newModule.eStage = ConvertShaderStageToVulkanFormat(pConfig.eStage);

	// Create a debug label.
	VULKAN_RENDERER_NAME_OBJECT(pRenderingContext, pConfig.pszModuleName, VK_OBJECT_TYPE_SHADER_MODULE, newModule.hModule);

	// Add the module to the shader.
	pOutModules.PushBack(newModule);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Gets a Vulkan-specific shader topology type from a generic type.
// Input:	inStage - The generic shader topology type.
// Output:	The Vulkan-specific shader topology type flag.
//-------------------------------------------------------------------------
static VkPrimitiveTopology GetVulkanTopologyType(unsigned int inType)
{
	switch (inType)
	{
	case SHADER_TOPOLOGY_TYPE_LINE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	default:
	case SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST:
	case SHADER_TOPOLOGY_TYPE_UNDEFINED:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

//-------------------------------------------------------------------------
// Purpose: Gets a Vulkan-specific shader topology type from a generic type.
// Input:	inStage - The generic shader topology type.
// Output:	The Vulkan-specific shader topology type flag.
//-------------------------------------------------------------------------
static VkFormat GetVertexFormatFromType(ShaderAttribute_t pType)
{
	switch (pType)
	{
	case ShaderAttribute_t::Int32:
		return VK_FORMAT_R32_SINT;
	case ShaderAttribute_t::Vector2:
		return VK_FORMAT_R32G32_SFLOAT;
	case ShaderAttribute_t::Vector3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case ShaderAttribute_t::Vector4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		LOG_WARN("Failed to parse vertex type, returning undefined.");
		return VK_FORMAT_UNDEFINED;
	}
}

//-------------------------------------------------------------------------
// Purpose: Creates a Vulkan shader from the given configuration.
// Input:	pShader - The generic front-end shader that will own this shader.
//			pConfig - The configuration for this shader.
// Output:	Returns true on successful creation, otherwise false.
//-------------------------------------------------------------------------
bool CVulkanRenderer::CreateShader(Shader* pShader, ShaderConfig_t& pConfig)
{
	// Get the internal shader data.
	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];

	// Piece together the shader modules.
	Utl::CVector<VulkanShaderModule_t>				modules{};
	Utl::CVector<VkPipelineShaderStageCreateInfo>	shaderStageInfos{};
	modules.Resize(pConfig.moduleConfigs.Count());
	for (int stageIdx = 0; stageIdx < pConfig.moduleConfigs.Count(); stageIdx++)
	{
		if (!CreateShaderModule(*this, pInternalShader, pConfig.moduleConfigs[stageIdx], modules))
		{
			LOG_ERROR("Failed to create shader module for shader '%s'. Shader creation aborted.", pConfig.pszShaderName);
			// TODO: Make sure to release any allocated resoures!
			return false;
		}
	}

	// Use the created modules as stages for the pipeline.
	shaderStageInfos.Resize(modules.Count());
	for (int stageIdx = 0; stageIdx < modules.Count(); stageIdx++)
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = modules[stageIdx].eStage;
		stageInfo.module = modules[stageIdx].hModule;
		stageInfo.pName = "main";	// All shader entrypoints are 'main()'.

		shaderStageInfos.PushBack(stageInfo);
	}

	// Vertex input
	// For now, we only need one binding.
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = pConfig.vertexStride;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Parse the vertex attributes from the config.
	size_t	attribCount = pConfig.vertexAttributes.Count();
	size_t	runningOffset{ 0 };
	Utl::CVector<VkVertexInputAttributeDescription> vertexAttributes(attribCount);
	for (int attribIdx = 0; attribIdx < attribCount; attribIdx++)
	{
		VkVertexInputAttributeDescription newAttribute{};
		newAttribute.binding = 0;
		newAttribute.location = attribIdx;
		newAttribute.format = GetVertexFormatFromType(pConfig.vertexAttributes[attribIdx].type);
		newAttribute.offset = (uint32_t)runningOffset;

		vertexAttributes.PushBack(newAttribute);
		runningOffset += pConfig.vertexAttributes[attribIdx].stride;
	}

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attribCount;
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.Data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = GetVulkanTopologyType(pConfig.eTopologyTypes);
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Dynamic state
	VkDynamicState						dynamicStates[2]{};
	VkPipelineDynamicStateCreateInfo	dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	// Viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_Swapchain.m_SwapchainExtent.width;
	viewport.height = (float)m_Swapchain.m_SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain.m_SwapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizerInfo.depthBiasClamp = VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;	// Set this to MODE_LINE for wireframe.
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisamplingState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisamplingState.sampleShadingEnable = VK_FALSE;
	multisamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if (pConfig.bDepthTestingEnabled)
	{
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;
	}

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = (pConfig.bColorBlendingEnabled) ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;

	// Setup uniform resources
	uint32_t iPerFrameUniformCount{ 0 };	// NOTE: Does not include samplers.
	uint32_t iPerFrameSamplerCount{ 0 };
	uint32_t iPerMaterialUniformCount{ 0 };	// Note: Does not include samplers.
	uint32_t iPerMaterialSamplerCount{ 0 };
	uint32_t iPerDrawUniformCount{ 0 };

	for (int idx = 0; idx < pConfig.uniformConfigs.Count(); idx++)
	{
		if (pConfig.uniformConfigs[idx].eFrequency == eShaderUniformGroup::PER_FRAME)
		{
			if (pConfig.uniformConfigs[idx].eType != ShaderAttribute_t::Sampler)
			{
				iPerFrameUniformCount += (1 * pConfig.uniformConfigs[idx].iArraySize);
			}
			else
			{
				iPerFrameSamplerCount += (1 * pConfig.uniformConfigs[idx].iArraySize);
			}
		}
		else if (pConfig.uniformConfigs[idx].eFrequency == eShaderUniformGroup::PER_MATERIAL)
		{
			if (pConfig.uniformConfigs[idx].eType != ShaderAttribute_t::Sampler)
			{
				iPerMaterialUniformCount += (1 * pConfig.uniformConfigs[idx].iArraySize);
			}
			else
			{
				iPerMaterialSamplerCount += (1 * pConfig.uniformConfigs[idx].iArraySize);
			}
		}
		else
		{
			iPerDrawUniformCount += (1 * pConfig.uniformConfigs[idx].iArraySize);
		}
	}

	// Descriptor pool
	VkDescriptorPoolSize descriptorPoolSizes[2]{};
	uint32_t			 descriptorPoolsNeeded{ 1 };

	// Uniform buffer.
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = kMaxVulkanInstances;

	// Image samplers.
	// TODO: SEPARATE SAMPLERS AND IMAGES.
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = kMaxVulkanInstances;

	// Only create a second descriptor pool if samplers will be used.
	if (iPerFrameSamplerCount + iPerMaterialSamplerCount > 0)
		descriptorPoolsNeeded++;

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.maxSets = kMaxVulkanInstances;
	descriptorPoolInfo.poolSizeCount = descriptorPoolsNeeded;
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;

	VK_CHECK(vkCreateDescriptorPool(m_Device.m_Device, &descriptorPoolInfo, m_Allocator, &pInternalShader->m_DescriptorPool));

	pInternalShader->m_DescriptorSetLayouts.Resize(2);
	// Per-frame descriptor set layout.
	{
		Utl::CVector<VkDescriptorSetLayoutBinding> perFrameDescriptorLayoutBindings{};

		// First is the uniform buffer -- this is always expected to exist.
		{
			VkDescriptorSetLayoutBinding perFrameUBOBinding{};
			perFrameUBOBinding.binding = 0;
			perFrameUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			perFrameUBOBinding.descriptorCount = 1;
			perFrameUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			perFrameDescriptorLayoutBindings.PushBack(perFrameUBOBinding);
		}

		// Next is per-frame samplers, only needed if images are actually being used at the per-frame level.
		if (iPerFrameSamplerCount > 0)
		{
			VkDescriptorSetLayoutBinding perFrameSamplerBinding{};
			perFrameSamplerBinding.binding = 1;
			perFrameSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	// TODO: Separate samplers and images
			perFrameSamplerBinding.descriptorCount = 32; // TODO: MAKE THIS A CONST SOMEWHERE
			perFrameSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			perFrameDescriptorLayoutBindings.PushBack(perFrameSamplerBinding);
		}

		// Create the per-frame descriptor set layout.
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.bindingCount = (uint32_t)perFrameDescriptorLayoutBindings.Count();
		descriptorSetLayoutInfo.pBindings = perFrameDescriptorLayoutBindings.Data();
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device.m_Device, &descriptorSetLayoutInfo, m_Allocator, &pInternalShader->m_DescriptorSetLayouts[0].m_Layout));
		pInternalShader->m_DescriptorSetLayouts[0].m_FrequencyLevel = eShaderUniformGroup::PER_FRAME;
	}

	// Per-material descriptor set layout.
	{
		Utl::CVector<VkDescriptorSetLayoutBinding> perMaterialDescriptorLayoutBindings{};

		// First is the uniform buffer -- this is always expected to exist.
		{
			VkDescriptorSetLayoutBinding perMaterialUBOBinding{};
			perMaterialUBOBinding.binding = 0;
			perMaterialUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			perMaterialUBOBinding.descriptorCount = 1;
			perMaterialUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			perMaterialDescriptorLayoutBindings.PushBack(perMaterialUBOBinding);
		}

		// Next is per-material samplers, only needed if images are actually being used at the per-frame level.
		if (iPerMaterialSamplerCount > 0)
		{
			VkDescriptorSetLayoutBinding perMaterialSamplerBinding{};
			perMaterialSamplerBinding.binding = 1;
			perMaterialSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	// TODO: Separate samplers and images
			perMaterialSamplerBinding.descriptorCount = iPerMaterialSamplerCount;
			perMaterialSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			perMaterialDescriptorLayoutBindings.PushBack(perMaterialSamplerBinding);
		}

		// Create the per-material descriptor set layout.
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.bindingCount = (uint32_t)perMaterialDescriptorLayoutBindings.Count();
		descriptorSetLayoutInfo.pBindings = perMaterialDescriptorLayoutBindings.Data();
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device.m_Device, &descriptorSetLayoutInfo, m_Allocator, &pInternalShader->m_DescriptorSetLayouts[1].m_Layout));
		pInternalShader->m_DescriptorSetLayouts[1].m_FrequencyLevel = eShaderUniformGroup::PER_MATERIAL;
	}

	// Setup uniform buffers.
	pInternalShader->m_UniformBuffers.Resize(m_MaxImagesInFlight);
	size_t iUBOSize = (pConfig.perFrameUBOStride) + (pConfig.perInstanceUBOStride * kMaxVulkanInstances);
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		if (!VulkanBuffer::CreateBuffer(s_VulkanContext, iUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, pInternalShader->m_UniformBuffers[idx]))
		{
			LOG_ERROR("Failed to create uniform buffer.");
			// TODO: Make sure to release any allocated resoures!
			return false;
		}
	}

	// Setup per-instance samplers.
	if (iPerMaterialSamplerCount > 0)
	{
		pInternalShader->m_InstanceSamplers.Resize(iPerMaterialSamplerCount);

		// TODO: Make this configurable.
		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = m_Device.m_DeviceProperties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		for (unsigned int idx = 0; idx < iPerMaterialSamplerCount; idx++)
		{
			VK_CHECK(vkCreateSampler(m_Device.m_Device, &samplerInfo, m_Allocator, &pInternalShader->m_InstanceSamplers[idx]));
		}
	}

	VkPushConstantRange pushConstants{};
	pushConstants.size = pConfig.perDrawUBOStride;
	pushConstants.offset = 0;
	pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Currently only used in the vertex shader.

	VkDescriptorSetLayout descriptorSetLayouts[2] = { pInternalShader->m_DescriptorSetLayouts[0].m_Layout, pInternalShader->m_DescriptorSetLayouts[1].m_Layout };

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = (uint32_t)pInternalShader->m_DescriptorSetLayouts.Capacity();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstants;
	VK_CHECK(vkCreatePipelineLayout(m_Device.m_Device, &pipelineLayoutInfo, m_Allocator, &pInternalShader->m_PipelineLayout));

	// Renderpass
	VulkanRenderPass* pRenderpass = reinterpret_cast<VulkanRenderPass*>(pConfig.m_pRendererView.m_pInternalData);

	// Graphics pipeline object
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	graphicsPipelineInfo.stageCount = (uint32_t)shaderStageInfos.Count();
	graphicsPipelineInfo.pStages = shaderStageInfos.Data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineInfo.pTessellationState = nullptr;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pRasterizationState = &rasterizerInfo;
	graphicsPipelineInfo.pMultisampleState = &multisamplingState;
	//graphicsPipelineInfo.pDepthStencilState = (pConfig.bDepthTestingEnabled) ? &depthStencil : nullptr;
	graphicsPipelineInfo.pDepthStencilState = &depthStencil; // TODO: Make this configurable through renderpasses w/o a depth attachment!
	graphicsPipelineInfo.pColorBlendState = &colorBlendState;
	graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
	graphicsPipelineInfo.layout = pInternalShader->m_PipelineLayout;
	graphicsPipelineInfo.renderPass = pRenderpass->m_Handle;
	graphicsPipelineInfo.subpass = 0;
	VK_CHECK(vkCreateGraphicsPipelines(m_Device.m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, m_Allocator, &pInternalShader->m_Pipeline));

	// We no longer need the shader modules.
	for (int stageIdx = 0; stageIdx < modules.Count(); stageIdx++)
	{
		vkDestroyShaderModule(m_Device.m_Device, modules[stageIdx].hModule, m_Allocator);
	}

	// Allocate the global descriptor set while we're here.
	pInternalShader->m_DescriptorSets.Resize(kMaxVulkanInstances + 1); // Max instances + 1 for the per-frame descriptors.
	VkDescriptorSetLayout* pGlobalLayout = nullptr;
	for (int layout = 0; layout < pInternalShader->m_DescriptorSetLayouts.Capacity(); layout++)
	{
		if (pInternalShader->m_DescriptorSetLayouts[layout].m_FrequencyLevel == eShaderUniformGroup::PER_FRAME)
		{
			pGlobalLayout = &pInternalShader->m_DescriptorSetLayouts[layout].m_Layout;
		}
	}

	if (!pGlobalLayout)
	{
		// No global resources were created, boot out now.
		return true;
	}

	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = pInternalShader->m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = pGlobalLayout;

		VK_CHECK(vkAllocateDescriptorSets(m_Device.m_Device, &allocInfo, &pInternalShader->m_DescriptorSets[0].m_DescriptorSets[idx]));	// Per-frame descriptors are always first.
	}
	pInternalShader->m_DescriptorSets[0].m_Generation = 0;

	// Bind the descriptor sets to our uniform buffer.
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = pInternalShader->m_UniformBuffers[idx].m_Handle;
		bufferInfo.offset = 0; // Per-frame is first!
		bufferInfo.range = pConfig.perFrameUBOStride;

		VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptorWrite.dstSet = pInternalShader->m_DescriptorSets[0].m_DescriptorSets[idx];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_Device.m_Device, 1, &descriptorWrite, 0, nullptr);
	}

	// We'll need then when setting up uniform descriptors.
	pShader->m_iUniformBufferAlignmentRequirement = m_Device.m_DeviceProperties.limits.minUniformBufferOffsetAlignment;

	// Bump up the next free descriptor set index.
	pInternalShader->m_NextFreeDescriptorSetIndex = 1;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases Vulkan shader resources for the provided shader.
// Input:	pShader - The generic front-end shader to release internal resources for.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::DestroyShader(Shader* pShader)
{
	// Make sure no resources are in use.
	vkDeviceWaitIdle(m_Device.m_Device);
	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];
	CAssert(pInternalShader);

	// Pipeline
	if (pInternalShader->m_Pipeline)
		vkDestroyPipeline(m_Device.m_Device, pInternalShader->m_Pipeline, m_Allocator);

	// Pipeline layout
	if (pInternalShader->m_PipelineLayout)
		vkDestroyPipelineLayout(m_Device.m_Device, pInternalShader->m_PipelineLayout, m_Allocator);

	// Per-Instance samplers
	for (int sampIdx = 0; sampIdx < pInternalShader->m_InstanceSamplers.Capacity(); sampIdx++)
	{
		if (pInternalShader->m_InstanceSamplers[sampIdx])
			vkDestroySampler(m_Device.m_Device, pInternalShader->m_InstanceSamplers[sampIdx], m_Allocator);
	}

	// Uniform buffers
	for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
	{
		VulkanBuffer::DestroyBuffer(s_VulkanContext, pInternalShader->m_UniformBuffers[idx]);
	}

	// Descriptor set layouts
	for (int idx = 0; idx < pInternalShader->m_DescriptorSetLayouts.Capacity(); idx++)
	{
		vkDestroyDescriptorSetLayout(m_Device.m_Device, pInternalShader->m_DescriptorSetLayouts[idx].m_Layout, m_Allocator);
	}

	// Descriptor pool
	if (pInternalShader->m_DescriptorPool)
		vkDestroyDescriptorPool(m_Device.m_Device, pInternalShader->m_DescriptorPool, m_Allocator);
}

//-------------------------------------------------------------------------
// Purpose: Acquires the shader resources needed for rendering an instance with the provided shader.
// Input:	pShader - The generic front-end shader to acquire resources for.
//			pOutInstanceID - A pointer to hold the instance of the new instance ID.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::AcquireShaderInstanceResources(Shader* pShader, unsigned int* pOutInstanceID)
{
	CAssert(pShader->m_iInternalHandle != kInvalidID_U32 && pOutInstanceID);
	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];

	// Find the instance-level descriptor set layout.
	VkDescriptorSetLayout instanceDescriptorSetLayout{};

	for (int layoutIdx = 0; layoutIdx < pInternalShader->m_DescriptorSetLayouts.Capacity(); layoutIdx++)
	{
		if (pInternalShader->m_DescriptorSetLayouts[layoutIdx].m_FrequencyLevel == eShaderUniformGroup::PER_MATERIAL)
		{
			// We want this one.
			instanceDescriptorSetLayout = pInternalShader->m_DescriptorSetLayouts[layoutIdx].m_Layout;
		}
	}

	// Bind resources once per image in flight.
	for (unsigned int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
	{
		// Allocate the descriptor set.
		VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = pInternalShader->m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &instanceDescriptorSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(m_Device.m_Device, &allocInfo, &pInternalShader->m_DescriptorSets[pInternalShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx]));

		Utl::CVector<VkWriteDescriptorSet>	descriptorWrites{};
		Utl::CVector<VkDescriptorImageInfo>	imageDescriptors{};
		VkDescriptorBufferInfo				uboDescriptor{};

		// Image samplers do not count towards UBO stride. Therefore, if UBO stride > 0, then we need to bind UBO resources.
		if (pShader->m_iPerInstanceUBOSize > 0)
		{
			uboDescriptor.buffer = pInternalShader->m_UniformBuffers[frameIdx].m_Handle;
			uboDescriptor.offset = pShader->m_iPerFrameUBOSize + (pShader->m_iPerInstanceUBOSize * pInternalShader->m_NextFreeDescriptorSetIndex);	// Add the per-frame UBO stride onto the front because those resources live at the front of the buffer.
			uboDescriptor.range = pShader->m_iPerInstanceUBOSize;

			if (uboDescriptor.offset % pShader->m_iUniformBufferAlignmentRequirement != 0)
			{
				// We need to move forward some to align properly.
				size_t bytesToMoveForward = pShader->m_iUniformBufferAlignmentRequirement - (uboDescriptor.offset % pShader->m_iUniformBufferAlignmentRequirement);
				uboDescriptor.offset += bytesToMoveForward;
			}

			VkWriteDescriptorSet uboDescriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			uboDescriptorWrite.dstSet = pInternalShader->m_DescriptorSets[pInternalShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
			uboDescriptorWrite.dstBinding = 0;
			uboDescriptorWrite.descriptorCount = 1;
			uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboDescriptorWrite.pBufferInfo = &uboDescriptor;

			descriptorWrites.PushBack(uboDescriptorWrite);
		}

		TextureHandle_t	hDefaultWhiteTexture = g_pTextureSystem->GetDefaultWhiteTexture();

		// Allocate image samplers (if used), and point them at the default texture for now.
		for (int imageIdx = 0; imageIdx < pInternalShader->m_InstanceSamplers.Capacity(); imageIdx++)
		{
			VkDescriptorImageInfo imageDescriptor{};
			imageDescriptor.sampler = pInternalShader->m_InstanceSamplers[imageIdx];
			imageDescriptor.imageView = m_pInternalTextureData[hDefaultWhiteTexture].m_Image.m_View;
			imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDescriptors.PushBack(imageDescriptor);

			VkWriteDescriptorSet imageDescriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			imageDescriptorWrite.dstSet = pInternalShader->m_DescriptorSets[pInternalShader->m_NextFreeDescriptorSetIndex].m_DescriptorSets[frameIdx];
			imageDescriptorWrite.dstBinding = 1;
			imageDescriptorWrite.descriptorCount = 1;
			imageDescriptorWrite.dstArrayElement = imageIdx;
			imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorWrite.pImageInfo = &imageDescriptors.Back();
			descriptorWrites.PushBack(imageDescriptorWrite);
		}

		// Update the descriptor sets.
		vkUpdateDescriptorSets(m_Device.m_Device, (uint32_t)descriptorWrites.Count(), descriptorWrites.Data(), 0, nullptr);
	}

	// Bump up the next available descriptor set index.
	*pOutInstanceID = pInternalShader->m_NextFreeDescriptorSetIndex;
	pInternalShader->m_NextFreeDescriptorSetIndex++;
}

//-------------------------------------------------------------------------
// Purpose: Binds a given shader for use in rendering ops.
// Input:	pShader - The shader to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::BindShader(Shader* pShader)
{
	CAssert(m_bCurrentlyRendering[m_ImageIndex]); // This operation can only happen during renderpasses.
	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];
	vkCmdBindPipeline(m_GraphicsCommandBuffers[m_ImageIndex].m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pInternalShader->m_Pipeline);

	// Save the internal pointer for use later.
	m_pCurrentlyBoundShader = pInternalShader;
	m_CurrentlyBoundShaderIndex = pShader->m_iInternalHandle;
}

//-------------------------------------------------------------------------
// Purpose: Binds the provided shader's per-frame uniforms.
// Input:	pShader - The shader whose uniforms are to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::BindPerFrameShaderDescriptors(Shader* pShader)
{
	CAssert(m_bCurrentlyRendering[m_ImageIndex]); // This operation can only happen during renderpasses.

	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];
	uint8_t descriptorSetIndex{ 0 };	// Per-frame descriptors are always first.

	vkCmdBindDescriptorSets(m_GraphicsCommandBuffers[m_ImageIndex].m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pInternalShader->m_PipelineLayout, 0, 1, &pInternalShader->m_DescriptorSets[descriptorSetIndex].m_DescriptorSets[m_ImageIndex], 0, nullptr);
}

//-------------------------------------------------------------------------
// Purpose: Binds the provided shader's per-instance uniforms (for the provided instance).
// Input:	pShader - The shader whose uniforms are to be bound.
//			hInstanceID - The instance to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::BindPerInstanceShaderDescriptors(Shader* pShader, unsigned int hInstanceID)
{
	CAssert(m_bCurrentlyRendering[m_ImageIndex]); // This operation can only happen during renderpasses.

	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];
	vkCmdBindDescriptorSets(m_GraphicsCommandBuffers[m_ImageIndex].m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pInternalShader->m_PipelineLayout, 1, 1, &pInternalShader->m_DescriptorSets[hInstanceID].m_DescriptorSets[m_ImageIndex], 0, nullptr);
}

//-------------------------------------------------------------------------
// Purpose: Sets a particular shader uniform.
// Input:	pShader - The shader whose uniform to set.
//			pUniform - The uniform to be set.
//			iUniformArrayIndex - The array index of the uniform to set. Some uniforms can be arrays, others just single values.
//			hInstanceID - The instance to be set, this only matters for per-instance uniforms. Not relevant for other groups.
//			pData - An opaque pointer to the value to be set.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::SetShaderUniform(Shader* pShader, ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData)
{
	VulkanShaderInternal* pInternalShader = &m_pInternalShaderData[pShader->m_iInternalHandle];
	VkDeviceSize uniformOffset{ 0 };
	VkDeviceSize uniformStride{ 0 };

	// Textures/samplers are handled differently from uniform data.
	if (pUniform->eType != ShaderAttribute_t::Sampler)
	{
		switch (pUniform->eGroup)
		{
		case eShaderUniformGroup::PER_FRAME:
			uniformOffset = pUniform->Offset;
			uniformStride = pUniform->Stride;
			break;

		case eShaderUniformGroup::PER_MATERIAL:
			uniformOffset = pShader->m_iPerFrameUBOSize + (pShader->m_iPerInstanceUBOSize * hInstanceID);
			// Make sure it's aligned.
			if (uniformOffset % pShader->m_iUniformBufferAlignmentRequirement != 0)
			{
				// We need to move forward some to align properly.
				size_t bytesToMoveForward = pShader->m_iUniformBufferAlignmentRequirement - (uniformOffset % pShader->m_iUniformBufferAlignmentRequirement);
				uniformOffset += bytesToMoveForward;
			}
			uniformOffset += pUniform->Offset;
			uniformStride = pUniform->Stride;
			break;

		case eShaderUniformGroup::PER_DRAW:
			// Push-constants!
			uniformOffset = pUniform->Offset;
			uniformStride = pUniform->Stride;
			vkCmdPushConstants(m_GraphicsCommandBuffers[m_ImageIndex].m_Handle, pInternalShader->m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, (uint32_t)uniformOffset, (uint32_t)uniformStride, pData);

			return;	// We need to early-out here because this data is not going to get copied to a buffer.

		default:
			CAssert(0);
			break;
		}
		//CAssert(uniformOffset != 256);

		// Copy data over to the GPU.
		for (unsigned int idx = 0; idx < m_MaxImagesInFlight; idx++)
		{
			void* pUniformData{};
			vkMapMemory(m_Device.m_Device, pInternalShader->m_UniformBuffers[idx].m_Memory, uniformOffset, uniformStride, 0, &pUniformData);
			memcpy(pUniformData, pData, uniformStride);
			vkUnmapMemory(m_Device.m_Device, pInternalShader->m_UniformBuffers[idx].m_Memory);
		}
	}
	else
	{
		// The opaque data is just a handle to the specific texture data.
		int imageIndex = *(int*)pData;

		// Point the appropriate descriptor to the texture's image data.
		for (unsigned int frameIdx = 0; frameIdx < m_MaxImagesInFlight; frameIdx++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = pInternalShader->m_InstanceSamplers[iUniformArrayIndex];
			imageInfo.imageView = m_pInternalTextureData[imageIndex].m_Image.m_View;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = pInternalShader->m_DescriptorSets[hInstanceID].m_DescriptorSets[frameIdx];
			descriptorWrite.dstBinding = 1; // Images always live at index 1.
			descriptorWrite.dstArrayElement = iUniformArrayIndex;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(m_Device.m_Device, 1, &descriptorWrite, 0, nullptr);
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Gets a Vulkan-specific format for a shader attribute.
// Input:	inAttribute - The generic shader attribute type.
// Output:	The Vulkan-specific format.
//-------------------------------------------------------------------------
static VkFormat VertexAttributeToVulkanType(ShaderAttribute_t inAttribute)
{
	switch (inAttribute)
	{
	case ShaderAttribute_t::Vector2:
		return VK_FORMAT_R32G32_SFLOAT;
	case ShaderAttribute_t::Vector3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case ShaderAttribute_t::Vector4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

//-------------------------------------------------------------------------
// Purpose: Gets the stride for a shader attribute.
// Input:	inAttribute - The generic shader attribute type.
// Output:	The stride of the attribute.
//-------------------------------------------------------------------------
static uint32_t GetSizeOfVertexAttribute(ShaderAttribute_t inAttribute)
{
	switch (inAttribute)
	{
	case ShaderAttribute_t::Vector2:
		return sizeof(Vector2D);
	case ShaderAttribute_t::Vector3:
		return sizeof(Vector3D);
	case ShaderAttribute_t::Vector4:
		return sizeof(Vector4D);
	case ShaderAttribute_t::Matrix4x4:
		return sizeof(Matrix4x4);
	default:
		return 0;
	}
}

//-------------------------------------------------------------------------
// Purpose: Acquires the renderer-specific resources for rendering geometry.
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometryConfig - The specific geometry data.
//			pOutInstance - A structure that holds the instance of geometry after acquisition.
// Output:	none.
// TODO: THIS SHOULDN'T RETURN ANYTHING!
//-------------------------------------------------------------------------
bool CVulkanRenderer::AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	VulkanBuffer* pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer* pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	pOutInstance.vertexDataOffset = pInternalVertexBuffer->m_BytesAllocated;
	pOutInstance.vertexCount = pGeometryConfig.Vertices.Count();

	VulkanCommandBuffer cmdBuffer{};
	cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Create a staging buffer for transfer to the GPU.
	VulkanBuffer stagingBuffer{};
	VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer);

	stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data());

	size_t finalBufferOffset = pInternalVertexBuffer->m_BytesAllocated;
	// Use the device local buffer's freelist to get the dstOffset.
	if (pInternalVertexBuffer->m_bUsesFreelist)
	{
		finalBufferOffset = pInternalVertexBuffer->m_Freelist.Allocate(stagingBuffer.m_BufferSize);
	}

	pOutInstance.vertexDataOffset = finalBufferOffset;
	pOutInstance.vertexCount = pGeometryConfig.Vertices.Count();

	// Copy over to the device-local buffer.
	VkBufferCopy copyRegion{};
	copyRegion.size = stagingBuffer.m_BufferSize;
	copyRegion.dstOffset = pOutInstance.vertexDataOffset;
	vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, pInternalVertexBuffer->m_Handle, 1, &copyRegion);

	cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);

	// TODO: Handle index buffers as well...
	if (pGeometryConfig.Indices.Count() != 0) // TODO: Better check could be used here.
	{
		pOutInstance.indexDataOffset = pInternalIndexBuffer->m_BytesAllocated;
		pOutInstance.indexCount = pGeometryConfig.Indices.Count();

		cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

		VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer);
		stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), pGeometryConfig.Indices.Data());
		finalBufferOffset = pInternalIndexBuffer->m_BytesAllocated;
		if (pInternalIndexBuffer->m_bUsesFreelist)
		{
			finalBufferOffset = pInternalIndexBuffer->m_Freelist.Allocate(stagingBuffer.m_BufferSize);
		}

		copyRegion.size = stagingBuffer.m_BufferSize;
		copyRegion.dstOffset = finalBufferOffset;
		vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, pInternalIndexBuffer->m_Handle, 1, &copyRegion);

		cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);
		VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);
	}

	pInternalVertexBuffer->m_BytesAllocated += pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride();
	pInternalIndexBuffer->m_BytesAllocated += pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride();

	// Copy over element strides.
	pOutInstance.vertexDataStride = pGeometryConfig.Vertices.Stride();
	pOutInstance.indexDataStride = pGeometryConfig.Indices.Stride();

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Acquires the renderer-specific resources for rendering geometry.
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometryConfig - The specific geometry data.
//			pOutInstance - A structure that holds the instance of geometry after acquisition.
// Output:	none.
// TODO: THIS SHOULDN'T RETURN ANYTHING!
//-------------------------------------------------------------------------
bool CVulkanRenderer::AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	VulkanBuffer* pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer* pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	VulkanCommandBuffer cmdBuffer{};
	cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Create a staging buffer for transfer to the GPU.
	VulkanBuffer stagingBuffer{};
	VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer);
	stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data());

	size_t finalBufferOffset = pInternalVertexBuffer->m_BytesAllocated;
	// Use the device local buffer's freelist to get the dstOffset.
	if (pInternalVertexBuffer->m_bUsesFreelist)
	{
		finalBufferOffset = pInternalVertexBuffer->m_Freelist.Allocate(stagingBuffer.m_BufferSize);
	}

	pOutInstance.vertexDataOffset = finalBufferOffset;
	pOutInstance.vertexCount = pGeometryConfig.Vertices.Count();

	//pOutInstance.indexDataOffset = m_UIIndexBuffer.m_BytesAllocated;
	//pOutInstance.indexDataSize = pGeometryConfig.Indices.Count();

	// Copy over to the device-local buffer.
	VkBufferCopy copyRegion{};
	copyRegion.size = stagingBuffer.m_BufferSize;
	copyRegion.dstOffset = finalBufferOffset;
	vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, pInternalVertexBuffer->m_Handle, 1, &copyRegion);

	cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);

	// TODO: Handle index buffers as well...
	if (pGeometryConfig.Indices.Count() != 0) // TODO: Better check could be used here.
	{
		cmdBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

		VulkanBuffer::CreateBuffer(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer);
		stagingBuffer.UploadDataRange(s_VulkanContext, pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride(), pGeometryConfig.Indices.Data());

		finalBufferOffset = pInternalIndexBuffer->m_BytesAllocated;
		if (pInternalIndexBuffer->m_bUsesFreelist)
		{
			finalBufferOffset = pInternalIndexBuffer->m_Freelist.Allocate(stagingBuffer.m_BufferSize);
		}

		pOutInstance.indexDataOffset = finalBufferOffset;
		pOutInstance.indexCount = pGeometryConfig.Indices.Count();

		copyRegion.size = stagingBuffer.m_BufferSize;
		copyRegion.dstOffset = finalBufferOffset;
		vkCmdCopyBuffer(cmdBuffer.m_Handle, stagingBuffer.m_Handle, pInternalIndexBuffer->m_Handle, 1, &copyRegion);

		cmdBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);
		VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);
	}

	pInternalVertexBuffer->m_BytesAllocated += pGeometryConfig.Vertices.Count() * pGeometryConfig.Vertices.Stride();
	pInternalIndexBuffer->m_BytesAllocated += pGeometryConfig.Indices.Count() * pGeometryConfig.Indices.Stride();

	// Copy over element strides.
	pOutInstance.vertexDataStride = pGeometryConfig.Vertices.Stride();
	pOutInstance.indexDataStride = pGeometryConfig.Indices.Stride();

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases the renderer-specific resources for geometry.
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometry - The geometry instance to be released.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry)
{
	VulkanBuffer* pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer* pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	// Mark the range in our renderbuffer as being free.
	pInternalVertexBuffer->FreeDataRange(s_VulkanContext, pGeometry.vertexDataOffset, pGeometry.vertexCount * pGeometry.vertexDataStride);

	// Free index data as well (if applicable)
	if (pGeometry.indexCount != kInvalidID_U64)
	{
		pInternalIndexBuffer->FreeDataRange(s_VulkanContext, pGeometry.indexDataOffset, pGeometry.indexCount * pGeometry.indexDataStride);
	}
}

//-------------------------------------------------------------------------
// Purpose: Updates the vertex/index data associated with a piece of geometry.
// Input:	pVertexBuffer - The vertex buffer associated with this geometry.
//			pIndexBuffer - The index buffer associated with this geometry.
//			pGeometry - The geometry instance to be updated.
//			pNewGeometryConfig - The new geometry data to overwrite the existing data.
// Output:	none.
//-------------------------------------------------------------------------
void CVulkanRenderer::UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig) 
{
	VulkanBuffer*		pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer*		pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	VulkanCommandBuffer	commandBuffer{};

	VulkanBuffer		stagingBuffer{};
	size_t				iVertexDataSize = pNewGeometryConfig.Vertices.Count() * pNewGeometryConfig.Vertices.Stride();
	size_t				iIndexDataSize = pNewGeometryConfig.Indices.Count() * pNewGeometryConfig.Indices.Stride();

	bool				bVertexReallocRequired = pGeometry.vertexCount != pNewGeometryConfig.Vertices.Count();
	bool				bIndexReallocRequired = (iIndexDataSize > 0) && pGeometry.indexCount != pNewGeometryConfig.Indices.Count() * pNewGeometryConfig.Indices.Stride();

	// Vertex data is required.
	if (iVertexDataSize < 1)
	{
		CAssert(0);
	}

	// Create a staging buffer for GPU upload.
	if (!VulkanBuffer::CreateBuffer(s_VulkanContext, iVertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer))
	{
		LOG_ERROR("Failed to create staging buffer for geometry update. Nothing was done.");
		return;
	}

	// Upload the new vertex data to the staging buffer.
	stagingBuffer.UploadDataRange(s_VulkanContext, iVertexDataSize, pNewGeometryConfig.Vertices.Data());

	// Create a command buffer to handle the upload.
	commandBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Free/allocate memory if needed, otherwise just overwrite the existing range.
	if (bVertexReallocRequired)
	{
		// We need to free and reallocate the memory to fit this in.
		if (!pInternalVertexBuffer->m_bUsesFreelist)
		{
			LOG_WARN("UpdateGeometryResources was provided with a vertex buffer that does not use a freelist. There will be a memory leak.");

			// Just allocate in a linear fashion.
			pGeometry.vertexDataOffset = pInternalVertexBuffer->m_BytesAllocated;
		}
		else
		{
			pInternalVertexBuffer->FreeDataRange(s_VulkanContext, pGeometry.vertexDataOffset, pGeometry.vertexCount * pGeometry.vertexDataStride);

			// Allocate from the freelist.
			pGeometry.vertexDataOffset = pInternalVertexBuffer->m_Freelist.Allocate(iVertexDataSize);
		}

		pGeometry.vertexCount = pNewGeometryConfig.Vertices.Count();
		pGeometry.vertexDataStride = pNewGeometryConfig.Vertices.Stride();
	}

	// Copy the data to the device-local buffer.
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.size = iVertexDataSize;
	copyRegion.dstOffset = pGeometry.vertexDataOffset;
	vkCmdCopyBuffer(commandBuffer.m_Handle, stagingBuffer.m_Handle, pInternalVertexBuffer->m_Handle, 1, &copyRegion);

	commandBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Clear out the staging buffer, in case we need to upload index data.
	stagingBuffer.FreeDataRange(s_VulkanContext, 0, iVertexDataSize);

	// Now handle the index data, if applicable.
	if (iIndexDataSize > 0)
	{
		VulkanBuffer::Resize(s_VulkanContext, &stagingBuffer, iIndexDataSize);

		// Upload the new index data to the staging buffer.
		stagingBuffer.UploadDataRange(s_VulkanContext, iIndexDataSize, pNewGeometryConfig.Indices.Data());

		// Create a command buffer to handle the upload.
		commandBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

		// Free/allocate memory if needed, otherwise just overwrite the existing range.
		if (bIndexReallocRequired)
		{
			// We need to free and reallocate the memory to fit this in.
			if (!pInternalIndexBuffer->m_bUsesFreelist)
			{
				LOG_WARN("UpdateGeometryResources was provided with an index buffer that does not use a freelist. There will be a memory leak.");

				// Just allocate in a linear fashion.
				pGeometry.indexDataOffset = pInternalIndexBuffer->m_BytesAllocated;
			}
			else
			{
				pInternalIndexBuffer->FreeDataRange(s_VulkanContext, pGeometry.indexDataOffset, pGeometry.indexCount * pGeometry.indexDataStride);

				// Allocate from the freelist.
				pGeometry.indexDataOffset = pInternalIndexBuffer->m_Freelist.Allocate(iIndexDataSize);
			}

			pGeometry.indexCount = pNewGeometryConfig.Indices.Count();
			pGeometry.indexDataStride = pNewGeometryConfig.Indices.Stride();
		}

		// Copy the data to the device-local buffer.
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.size = iIndexDataSize;
		copyRegion.dstOffset = pGeometry.indexDataOffset;
		vkCmdCopyBuffer(commandBuffer.m_Handle, stagingBuffer.m_Handle, pInternalIndexBuffer->m_Handle, 1, &copyRegion);

		commandBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);
	}
	else
	{
		// If no index data is present in the new config, simply free the existing range.
		if (pGeometry.indexCount != kInvalidID_U64)
		{
			pInternalIndexBuffer->FreeDataRange(s_VulkanContext, pGeometry.indexDataOffset, pGeometry.indexCount);
			pGeometry.indexCount = 0;
			pGeometry.indexDataOffset = kInvalidID_U64;
		}
	}

	// Destroy resources allocated for the upload.
	VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);
}

//-------------------------------------------------------------------------
// Purpose: Updates the vertex/index data associated with a piece of geometry.
// Input:	pVertexBuffer - The vertex buffer associated with this geometry.
//			pIndexBuffer - The index buffer associated with this geometry.
//			pGeometry - The geometry instance to be updated.
//			pNewGeometryConfig - The new geometry data to overwrite the existing data.
// Output:	none.
// TODO: Find a way to combine this and the above method.
//-------------------------------------------------------------------------
void CVulkanRenderer::UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig)
{
	VulkanBuffer* pInternalVertexBuffer = reinterpret_cast<VulkanBuffer*>(pVertexBuffer.m_pInternalData);
	VulkanBuffer* pInternalIndexBuffer = reinterpret_cast<VulkanBuffer*>(pIndexBuffer.m_pInternalData);

	VulkanCommandBuffer	commandBuffer{};

	VulkanBuffer		stagingBuffer{};
	size_t				iVertexDataSize = pNewGeometryConfig.Vertices.Count() * pNewGeometryConfig.Vertices.Stride();
	size_t				iIndexDataSize = pNewGeometryConfig.Indices.Count() * pNewGeometryConfig.Indices.Stride();

	bool				bVertexReallocRequired = pGeometry.vertexCount != pNewGeometryConfig.Vertices.Count();
	bool				bIndexReallocRequired = (iIndexDataSize > 0) && pGeometry.indexCount != pNewGeometryConfig.Indices.Count() * pNewGeometryConfig.Indices.Stride();

	// Vertex data is required.
	if (iVertexDataSize < 1)
	{
		CAssert(0);
	}

	// Create a staging buffer for GPU upload.
	if (!VulkanBuffer::CreateBuffer(s_VulkanContext, iVertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, stagingBuffer))
	{
		LOG_ERROR("Failed to create staging buffer for geometry update. Nothing was done.");
		return;
	}

	// Upload the new vertex data to the staging buffer.
	stagingBuffer.UploadDataRange(s_VulkanContext, iVertexDataSize, pNewGeometryConfig.Vertices.Data());

	// Create a command buffer to handle the upload.
	commandBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Free/allocate memory if needed, otherwise just overwrite the existing range.
	if (bVertexReallocRequired)
	{
		// We need to free and reallocate the memory to fit this in.
		if (!pInternalVertexBuffer->m_bUsesFreelist)
		{
			LOG_WARN("UpdateGeometryResources was provided with a vertex buffer that does not use a freelist. There will be a memory leak.");

			// Just allocate in a linear fashion.
			pGeometry.vertexDataOffset = pInternalVertexBuffer->m_BytesAllocated;
		}
		else
		{
			pInternalVertexBuffer->FreeDataRange(s_VulkanContext, pGeometry.vertexDataOffset, pGeometry.vertexCount * pGeometry.vertexDataStride);

			// Allocate from the freelist.
			pGeometry.vertexDataOffset = pInternalVertexBuffer->m_Freelist.Allocate(iVertexDataSize);
		}

		pGeometry.vertexCount = pNewGeometryConfig.Vertices.Count();
		pGeometry.vertexDataStride = pNewGeometryConfig.Vertices.Stride();
	}

	// Copy the data to the device-local buffer.
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.size = iVertexDataSize;
	copyRegion.dstOffset = pGeometry.vertexDataOffset;
	vkCmdCopyBuffer(commandBuffer.m_Handle, stagingBuffer.m_Handle, pInternalVertexBuffer->m_Handle, 1, &copyRegion);

	commandBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);

	// Clear out the staging buffer, in case we need to upload index data.
	stagingBuffer.FreeDataRange(s_VulkanContext, 0, iVertexDataSize);

	// Now handle the index data, if applicable.
	if (iIndexDataSize > 0)
	{
		VulkanBuffer::Resize(s_VulkanContext, &stagingBuffer, iIndexDataSize);

		// Upload the new index data to the staging buffer.
		stagingBuffer.UploadDataRange(s_VulkanContext, iIndexDataSize, pNewGeometryConfig.Indices.Data());

		// Create a command buffer to handle the upload.
		commandBuffer.CreateAndBeginSingleUse(s_VulkanContext, m_GraphicsCommandPool);

		// Free/allocate memory if needed, otherwise just overwrite the existing range.
		if (bIndexReallocRequired)
		{
			// We need to free and reallocate the memory to fit this in.
			if (!pInternalIndexBuffer->m_bUsesFreelist)
			{
				LOG_WARN("UpdateGeometryResources was provided with an index buffer that does not use a freelist. There will be a memory leak.");

				// Just allocate in a linear fashion.
				pGeometry.indexDataOffset = pInternalIndexBuffer->m_BytesAllocated;
			}
			else
			{
				pInternalIndexBuffer->FreeDataRange(s_VulkanContext, pGeometry.indexDataOffset, pGeometry.indexCount * pGeometry.indexDataStride);

				// Allocate from the freelist.
				pGeometry.indexDataOffset = pInternalIndexBuffer->m_Freelist.Allocate(iIndexDataSize);
			}

			pGeometry.indexCount = pNewGeometryConfig.Indices.Count();
			pGeometry.indexDataStride = pNewGeometryConfig.Indices.Stride();
		}

		// Copy the data to the device-local buffer.
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.size = iIndexDataSize;
		copyRegion.dstOffset = pGeometry.indexDataOffset;
		vkCmdCopyBuffer(commandBuffer.m_Handle, stagingBuffer.m_Handle, pInternalIndexBuffer->m_Handle, 1, &copyRegion);

		commandBuffer.EndSingleUse(s_VulkanContext, m_GraphicsCommandPool);
	}
	else
	{
		// If no index data is present in the new config, simply free the existing range.
		if (pGeometry.indexCount != kInvalidID_U64)
		{
			pInternalIndexBuffer->FreeDataRange(s_VulkanContext, pGeometry.indexDataOffset, pGeometry.indexCount);
			pGeometry.indexCount = 0;
			pGeometry.indexDataOffset = kInvalidID_U64;
		}
	}

	// Destroy resources allocated for the upload.
	VulkanBuffer::DestroyBuffer(s_VulkanContext, stagingBuffer);
}

//-------------------------------------------------------------------------
// Purpose: Checks to make sure that support for all required validation layers are present.
// Input:	expectedLayers - Validation layers that are required to be supported.
// Output:	Returns true if all validation layers are supported, otherwise false.
//-------------------------------------------------------------------------
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
			if (Utl::StringCompare(expectedLayers[expectedIndex], availableLayers[layerIndex].layerName))
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

//-------------------------------------------------------------------------
// Purpose: Checks to make sure that support for all required instance extensions are present.
// Input:	expectedExtensions - Instance extensions that are required to be supported.
// Output:	Returns true if all instance extensions are supported, otherwise false.
//-------------------------------------------------------------------------
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
			if (Utl::StringCompare(expectedExtensions[currentExt], availableExtensions[availableExt].extensionName))
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

//-------------------------------------------------------------------------
// Purpose: Debug logger callback function.
// Input:	messageSeverity - The severity of the message (duh!).
//			messageType - The type of the message (...)
//			pCallbackData - Contains the callback message.
//			pUserData - Unused.
// Output:	VkBool32.
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// Purpose: Debug logger callback function.
// Input:	messageSeverity - The severity of the message (duh!).
//			messageType - The type of the message (...)
//			pCallbackData - Contains the callback message.
//			pUserData - Unused.
// Output:	VkBool32.
//-------------------------------------------------------------------------
static bool RendererEvent_WindowResized(EventData_t data)
{
	s_VulkanContext.m_bWindowResized = true;

	// Let other listeners handle this event.
	return false;
}

//-------------------------------------------------------------------------
// Renderer-backend event handlers.
//-------------------------------------------------------------------------
static bool RendererEvent_KeyPressed(EventData_t data)
{
	return false;
}

// TODO: This should be removed!
void CVulkanRenderer::DestroyDefaultGeometry()
{
}