//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"
#include "EngineGlobals.h"
#include <Containers/CVector.h>

#include <vulkan/vulkan.h>

// Forward Declarations
template <typename T> class CVector;

class	VulkanDevice;
struct	SwapchainSupportDetails;
class	VulkanSwapchain;
class	VulkanRenderpass;
class	VulkanCommandPool;
class	VulkanCommandBuffer;
class	VulkanFramebuffer;
class	VulkanFence;
class	VulkanSemaphore;
class	CVulkanRenderer;

// Utility Macro for checking success with VkResult
#define VK_CHECK(expr) expr == VK_SUCCESS

// Maximum frames that can be worked with at once
const char MAX_FRAMES_IN_FLIGHT{ 3 };

// TODO: These should be temporary
static unsigned int g_NewWidth{ 0 };
static unsigned int g_NewHeight{ 0 };
static bool g_FramebufferResized{ false };

//=================================
// 
// Vulkan Device
// 
//=================================
struct DeviceQueueFamilyInfo_t
{
	unsigned char GraphicsQueueIndex{ INVALID_ID_U8 };
	unsigned char ComputeQueueIndex{ INVALID_ID_U8 };
	unsigned char TransferQueueIndex{ INVALID_ID_U8 };
	unsigned char PresentQueueIndex{ INVALID_ID_U8 };

	bool IsComplete()
	{
		if (IsConceptOccupied(&GraphicsQueueIndex) && IsConceptOccupied(&PresentQueueIndex) 
			&& IsConceptOccupied(&TransferQueueIndex) && IsConceptOccupied(&ComputeQueueIndex))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsConceptOccupied(unsigned char* queueIndex) { return *queueIndex != INVALID_ID_U8; }
	bool IsIndexOccupied(unsigned char index)
	{
		return (GraphicsQueueIndex == index) || (ComputeQueueIndex == index) || 
			(TransferQueueIndex == index) || (PresentQueueIndex == index);
	}

	size_t GetNumberOfValidQueues()
	{
		size_t numQueues{ 0 };

		if (IsConceptOccupied(&GraphicsQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&ComputeQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&TransferQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&PresentQueueIndex))
			numQueues++;

		return numQueues;
	}
};

class VulkanDevice
{
public:
	static bool CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice);
	static void DestroyDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pDevice);
	
	void		QuerySwapchainSupportDetails(CVulkanRenderer* pRenderingContext, SwapchainSupportDetails* pOutDetails);

private:
	bool		SelectPhysicalDevice(CVulkanRenderer* pRenderingContext);
	bool		QueryDeviceQueueFamilyProperties(CVulkanRenderer* pRenderingContext, VkPhysicalDevice& physicalDevice, DeviceQueueFamilyInfo_t* pOutQueueInfo);
	bool		QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice);

	bool		CreateLogicalDevice(CVulkanRenderer* pRenderingContext);

public:
	VkDevice			m_LogicalDevice{ VK_NULL_HANDLE };

	VkQueue				m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue				m_PresentQueue{ VK_NULL_HANDLE };
	VkQueue				m_TransferQueue{ VK_NULL_HANDLE };
	VkQueue				m_ComputeQueue{ VK_NULL_HANDLE };

	VkPhysicalDevice			m_PhysicalDevice{ VK_NULL_HANDLE };

	VkPhysicalDeviceProperties	m_PhysicalDeviceProperties{};
	VkPhysicalDeviceFeatures	m_PhysicalDeviceFeatures{};
	DeviceQueueFamilyInfo_t		m_PhysicalDeviceQueueFamilyInfo{};

	CVector<const char*>* m_DeviceExtensions;
};

//=================================
// 
// Vulkan Swapchain
// 
//=================================
struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR	surfaceCapabilities;
	CVector<VkSurfaceFormatKHR> formats;
	CVector<VkPresentModeKHR>	presentModes;
};

class VulkanSwapchain
{
public:
	static bool CreateSwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pOutSwapchain);
	static void DestroySwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain);

	bool		RecreateSwapchain(CVulkanRenderer* pRenderingContext, unsigned int newWidth, unsigned int newHeight);
	bool		Present(CVulkanRenderer* pRenderingContext, unsigned int imageIndex);

	bool		GetSwapchainImages(CVulkanRenderer* pRenderingContext);
	bool		CreateImageViews(CVulkanRenderer* pRenderingContext);

	VkSwapchainKHR			m_SwapchainHandle{ VK_NULL_HANDLE };
	SwapchainSupportDetails m_SwapchainSupport{};
	CVector<VkImage>		m_SwapchainImages;
	CVector<VkImageView>	m_SwapchainImageViews;

	// Framebuffers for rendering to the screen
	CVector<VulkanFramebuffer> m_Framebuffers;

	VkFormat			m_SwapchainImageFormat;
	VkExtent2D			m_SwapchainExtent;
};

//=================================
// 
// Vulkan Renderpass
// 
//=================================
class VulkanRenderpass
{
public:
	static bool		CreateRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pOutRenderpass);
	static void		DestroyRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pRenderpass);

	bool			Begin(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer, unsigned int imageIndex);
	void			End(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);

	VkRenderPass m_Handle{ VK_NULL_HANDLE };
};

//=================================
// 
// Vulkan Command Buffer
// 
//=================================
enum class eVulkanCommandBufferState
{
	/* The command buffer has just been allocated or has been reset to the initial state. Can only be set to record or freed. */
	COMMAND_BUFFER_STATE_INITIAL,
	/* The command buffer can be written to with vkCmd calls. */
	COMMAND_BUFFER_STATE_RECORDING,
	/* The command buffer has been written to and can be freed or submitted for execution. */
	COMMAND_BUFFER_EXECUTABLE,
	/* The command buffer has been submitted for queue submission and CANNOT be modified at this point. */
	COMMAND_BUFFER_STATE_PENDING,
	/* The command buffer has been invalidated through an operation that modified it's resources, can be freed or rest. */
	COMMAND_BUFFER_STATE_INVALID
};

class VulkanCommandPool
{
public:
	static bool		CreateCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pOutPool);
	static void		DestroyCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pPool);

public:
	VkCommandPool				m_Handle{ VK_NULL_HANDLE };
	VkCommandPoolCreateFlags	m_PoolFlags;
};

class VulkanCommandBuffer
{
public:
	static bool		CreateCommandBuffer(CVulkanRenderer* pRenderingContext, bool isPrimary, VulkanCommandBuffer* pOutCommandBuffer);
	static void		DestroyCommandBuffer(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);

	bool			Begin();
	void			End();
	void			Reset();
	bool			Submit(CVulkanRenderer* pRenderingContext);

public:
	VkCommandBuffer				m_Handle{ VK_NULL_HANDLE };
	eVulkanCommandBufferState	m_State{ eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INITIAL };

	// Is this a primary or secondary level command buffer?
	bool						m_bIsPrimary{ false };
};


//=================================
// 
// Vulkan Shader
// 
//=================================
class VulkanShader
{
public:
	VulkanShader()
		: m_VertModule{ VK_NULL_HANDLE }
		, m_FragModule{ VK_NULL_HANDLE }
		, m_Pipeline{ VK_NULL_HANDLE }
		, m_PipelineLayout{ VK_NULL_HANDLE } {}

	static bool		CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);
	static void		DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader);

	static bool		CreatePipeline(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);

	bool			Bind(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer);

private:
	VkShaderModule		m_VertModule;
	VkShaderModule		m_FragModule;

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;
};


//=================================
// 
// Vulkan Framebuffer
// 
//=================================
class VulkanFramebuffer
{
public:
	static bool		CreateFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pOutFramebuffer, unsigned int framebufferIndex);
	static void		DestroyFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pFramebuffer);

public:
	VkFramebuffer			m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Fence
// 
//=================================
class VulkanFence
{
public:
	static bool		CreateFence(CVulkanRenderer* pRenderingContext, bool bCreateSignaled, VulkanFence* pOutFence);
	static void		DestroyFence(CVulkanRenderer* pRenderingContext, VulkanFence* pFence);

public:
		VkFence	m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Semaphore
// 
//=================================
class VulkanSemaphore
{
public:
	// NOTE: This needs to be named 'CreateVulkanSemaphore' as there is an existing 'CreateSemaphore' macro in the Win32 API that I don't want to disturb.
	static bool		CreateVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pOutSemaphore);
	static void		DestroyVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pSemaphore);

public:
	VkSemaphore m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Rendering Context
// 
//=================================
class CVulkanRenderer : public IRendererBackend
{
public:
	CVulkanRenderer() {}

	bool		RendererBackend_Initialize() override;
	void		RendererBackend_Shutdown() override;
	bool		RendererBackend_Update();

private:
	bool		CreateVulkanInstance();
	bool		QueryInstanceExtensionSupport(CVector<const char*>& requiredExtensions);
	bool		QueryValidationLayerSupport(CVector<const char*>& requiredValidationLayers);

	bool		CreateDebugMessenger();
	void		DestroyDebugMessenger();

public:
	// Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// TODO: Implement
	VkAllocationCallbacks* m_Allocator{ nullptr };

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };

	// Surface
	VkSurfaceKHR	m_Surface{ VK_NULL_HANDLE };

	// Device
	VulkanDevice	m_Device{};

	// Swapchain
	VulkanSwapchain	m_Swapchain{};

	// Main Renderpass
	VulkanRenderpass	m_MainRenderpass{};

	// Command Buffers
	VulkanCommandPool	m_GraphicsCommandPool{};
	CVector<VulkanCommandBuffer> m_CommandBuffers;

	// Sync Objects
	CVector<VulkanFence> m_FrameInFlightFences;
	CVector<VulkanSemaphore> m_ImageAvailableSemaphores;
	CVector<VulkanSemaphore> m_RenderFinishedSemaphores;

	// Shaders
	VulkanShader		m_StandardShader;

	// Validation Layers
	CVector<const char*>* m_ValidationLayers{};
	bool		EnableValidationLayers()
	{
#ifdef _DEBUG
		return true;
#else
		return false;
#endif
	}

	unsigned char m_CurrentFrame{ 0 };

	unsigned int m_FramebufferWidth{ 0 };
	unsigned int m_FramebufferHeight{ 0 };
};

IRendererBackend* GetVulkanBackend();

