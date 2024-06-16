//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"

#include "Core/Logging.h"
#include <Containers/CVector.h>

#include <vulkan/vulkan.h>

// Forward declarations
class CVulkanRenderer;
struct DeviceQueueFamilyProperties_t;
class VulkanDevice;
class VulkanSwapchain;
struct SwapchainSupportDetails_t;
class VulkanRenderpass;


// 3 for triple-buffering.
const unsigned short MAX_FRAMES_IN_FLIGHT = 3;


//=================================
// 
// Vulkan Device
// 
//=================================
struct DeviceQueueFamilyProperties_t
{
	DeviceQueueFamilyProperties_t()
		: graphicsIndex{ 0 }
		, computeIndex{ 0 }
		, transferIndex{ 0 }
		, presentIndex{ 0 } {}

	uint32_t		graphicsIndex;
	uint32_t		computeIndex;
	uint32_t		transferIndex;
	uint32_t		presentIndex;

	bool IsComplete() const
	{
		return (graphicsIndex != -1) && (presentIndex != -1) && (computeIndex != -1) && (transferIndex != -1);
	}
};


class VulkanDevice
{
public:
	VulkanDevice() 
		: m_PhysicalDeviceHandle{ VK_NULL_HANDLE }
		, m_PhysicalDeviceProperties{}
		, m_PhysicalDeviceQueueIndices{}
		, m_LogicalDeviceHandle{ VK_NULL_HANDLE }
		, m_GraphicsQueue{ VK_NULL_HANDLE }
		, m_PresentQueue{ VK_NULL_HANDLE }
		, m_TransferQueue{ VK_NULL_HANDLE }
		, m_ComputeQueue{ VK_NULL_HANDLE }
		, m_bDeviceSupportsSwapchain{ false }
		, m_GraphicsCommandPool{ VK_NULL_HANDLE }
		, m_GraphicsCommandBuffers{ VK_NULL_HANDLE } {}

	bool	CreateVulkanDevice(CVulkanRenderer* renderingContext);
	void	DestroyLogicalDevice(CVulkanRenderer* renderingContext);

	bool	CreateCommandBuffers(CVulkanRenderer* renderingContext);
	void	DestroyCommandBuffers(CVulkanRenderer* renderingContext);

	bool	BeginCommandBuffer(VkCommandBuffer& commandBuffer);
	bool	EndCommandBuffer(VkCommandBuffer& commandBuffer);
	bool	SubmitCommandBuffer(CVulkanRenderer* renderingContext, VkCommandBuffer& commandBuffer, int currentImageIndex);

	VkDevice& GetLogicalDevice() { return m_LogicalDeviceHandle; }
	VkPhysicalDevice& GetPhysicalDevice() { return m_PhysicalDeviceHandle; }
	DeviceQueueFamilyProperties_t& GetPhysicalDeviceQueueIndices() { return m_PhysicalDeviceQueueIndices; }

	void QueryDeviceSwapchainSupport(CVulkanRenderer* renderingContext, VkPhysicalDevice device, VulkanSwapchain& swapchain);

	VkCommandPool& GetGraphicsCommandPool() { return m_GraphicsCommandPool; }
	VkCommandBuffer& GetGraphicsCommandBuffer(int index) { return m_GraphicsCommandBuffers[index]; }

	VkQueue GetPresentQueue() { return m_PresentQueue; }

private:
	bool	SelectPhysicalDevice(CVulkanRenderer* renderingContext);
	bool	PhysicalDeviceMeetsRequirements(CVulkanRenderer* renderingContext, VkPhysicalDevice& physicalDevice);
	void	GetQueueFamilyData(CVulkanRenderer* renderingContext);
	bool	QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice);

	bool	CreateLogicalDevice(CVulkanRenderer* renderingContext);

private:
	VkPhysicalDevice m_PhysicalDeviceHandle;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
	DeviceQueueFamilyProperties_t m_PhysicalDeviceQueueIndices;

	VkDevice m_LogicalDeviceHandle;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	VkQueue m_TransferQueue;
	VkQueue m_ComputeQueue;

	CVector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	bool m_bDeviceSupportsSwapchain;

	// Command Pool/Buffers
	VkCommandPool m_GraphicsCommandPool;
	VkCommandBuffer m_GraphicsCommandBuffers[MAX_FRAMES_IN_FLIGHT];
};


//=================================
// 
// Swapchain
// 
//=================================
struct SwapchainSupportDetails_t
{
	SwapchainSupportDetails_t()
		: capabilities{}
		, formats{}
		, presentModes{} {}

	VkSurfaceCapabilitiesKHR capabilities;
	CVector<VkSurfaceFormatKHR> formats;
	CVector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain()
		: m_SwapchainHandle{ VK_NULL_HANDLE }
		, m_SwapchainSupportDetails{}
		, m_SwapchainImages{}
		, m_SwapchainImageViews{}
		, m_SwapchainImageFormat{}
		, m_SwapchainExtent{}
		, m_Framebuffers{ VK_NULL_HANDLE }
		, m_FramebufferResized{ false } {}


	bool		CreateSwapchain(CVulkanRenderer* renderingContext);
	void		CleanupSwapchain(CVulkanRenderer* renderingContext);
	bool		RecreateSwapchain(CVulkanRenderer* renderingContext);

	void		GetSwapchainImages(CVulkanRenderer* renderingContext);
	bool		CreateImageViews(CVulkanRenderer* renderingContext);
	void		DestroyImageViews(CVulkanRenderer* renderingContext);

	bool		CreateFramebuffers(CVulkanRenderer* renderingContext);
	void		DestroyFramebuffers(CVulkanRenderer* renderingContext);

	bool		FramebufferPresent(CVulkanRenderer* renderingContext, unsigned int currImageIndex);

	VkSwapchainKHR& GetHandle() { return m_SwapchainHandle; }

	SwapchainSupportDetails_t& GetSwapchainSupportDetails() { return m_SwapchainSupportDetails; }

	const VkFormat&	GetImageFormat() const { return m_SwapchainImageFormat; }
	const VkExtent2D& GetExtent() const { return m_SwapchainExtent; }

	VkFramebuffer GetFramebuffer(int index) { return m_Framebuffers[index]; }

private:
	// Utility functions for choosing formats on swapchain creation.
	VkSurfaceFormatKHR	ChooseSwapchainSurfaceFormat(CVector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR	ChooseSwapchainPresentMode(CVector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D			ChooseSwapchainExtent(CVulkanRenderer* renderingContext, const VkSurfaceCapabilitiesKHR& capabilities);


private:
	VkSwapchainKHR	m_SwapchainHandle;
	SwapchainSupportDetails_t m_SwapchainSupportDetails;
	CVector<VkImage> m_SwapchainImages;
	CVector<VkImageView> m_SwapchainImageViews;

	VkFormat	m_SwapchainImageFormat;
	VkExtent2D	m_SwapchainExtent;

	// Framebuffers for outputting to the screen
	// NOTE: This will need to switch to a dynamic array if we
	// want to support switching between double and triple buffering at runtime
	VkFramebuffer m_Framebuffers[MAX_FRAMES_IN_FLIGHT];
	bool m_FramebufferResized;
};


//=================================
// 
// Renderpass
// 
//=================================
class VulkanRenderpass
{
public:
	VulkanRenderpass()
		: m_RenderpassHandle{ VK_NULL_HANDLE } {}

	static bool	CreateRenderpass(CVulkanRenderer* renderingContext, VulkanRenderpass* pOutRenderpass);
	static void	DestroyRenderpass(CVulkanRenderer* renderingContext, VulkanRenderpass* pOutRenderpass);

	bool		BeginRenderpass(CVulkanRenderer* renderingContext, VkCommandBuffer* pCommandBuffer);
	void		EndRenderpass(CVulkanRenderer* renderingContext, VkCommandBuffer* pCommandBuffer);

	VkRenderPass& GetHandle() { return m_RenderpassHandle; }

private:
	VkRenderPass	m_RenderpassHandle;
};


//=================================
// 
// Vulkan Rendering Context
// 
//=================================
class CVulkanRenderer : public IRendererBackend
{
public:
	CVulkanRenderer()
		: m_Instance{ VK_NULL_HANDLE }
		, m_DebugMessenger{ VK_NULL_HANDLE }
		, m_Surface{ VK_NULL_HANDLE }
		, m_Device{}
		, m_Swapchain{}
		, m_MainRenderpass{}
		, m_ImageAvailableSemaphores((size_t)MAX_FRAMES_IN_FLIGHT)
		, m_RenderFinishedSemaphores((size_t)MAX_FRAMES_IN_FLIGHT)
		, m_InFlightFrameFences((size_t)MAX_FRAMES_IN_FLIGHT) {}


	bool	RendererBackend_Initialize() override;
	void	RendererBackend_Shutdown() override;
	bool	RendererBackend_Update() override;

	void	SetFramebufferResized(bool bTrue) { m_FramebufferResized = bTrue; }

private:
	bool	CreateVulkanInstance();
	bool	CreateDebugMessenger();
	void	DestroyDebugMessenger() const;

	bool	CreateSyncObjects();
	void	DestroySyncObjects();

private:
	// Instance
	VkInstance		m_Instance;

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger;

	// Surface
	VkSurfaceKHR	m_Surface;

	// Vulkan Device
	VulkanDevice	m_Device;

	// Swapchain
	VulkanSwapchain	m_Swapchain;

	// Renderpass
	VulkanRenderpass m_MainRenderpass;

	// Sync Objects
	CVector<VkSemaphore>	m_ImageAvailableSemaphores;
	CVector<VkSemaphore>	m_RenderFinishedSemaphores;
	CVector<VkFence>		m_InFlightFrameFences;

	unsigned short m_CurrentImageIndex{ 0 };
	unsigned short m_CurrentFrame{ 0 };
	bool	m_FramebufferResized{ false };

public:
	VkInstance& GetVulkanInstance() { return m_Instance; }
	CVector<const char*>& GetValidationLayers() { return m_ValidationLayers; }
	// Only enabling Valdiation Layers on debug builds for now
#if _DEBUG
	bool	EnableValidationLayers() const { return true; }
#else
	bool	EnableValidationLayers() const { return false; }
#endif

	VkSurfaceKHR&		GetVulkanSurface() { return m_Surface; }
	VulkanDevice&		GetDevice() { return m_Device; }
	VulkanSwapchain&	GetSwapchain() { return m_Swapchain; }
	VulkanRenderpass&	GetRenderpass() { return m_MainRenderpass; }

	VkSemaphore&		GetImageAvailableSemaphore(int index) { return m_ImageAvailableSemaphores[index]; }
	VkSemaphore&		GetRenderFinishedSemaphore(int index) { return m_RenderFinishedSemaphores[index]; }
	VkFence&			GetInFlightFrameFence(int index)	{ return m_InFlightFrameFences[index]; }

	unsigned short		GetCurrentFrameIndex() { return m_CurrentImageIndex; }

private:

	bool	RequestValidationLayerSupport();

	CVector<const char*> m_ValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
};

static CVulkanRenderer VulkanRenderingContext;

IRendererBackend* GetVulkanBackend();

