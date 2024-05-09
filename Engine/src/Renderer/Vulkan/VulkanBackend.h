//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"

#include "Core/Logging.h"
#include <CVector.h>

#include <vulkan/vulkan.h>

// Forward declarations
class CVulkanRenderer;
struct DeviceQueueFamilyProperties_t;
class VulkanDevice;
class VulkanSwapchain;
struct SwapchainSupportDetails_t;


//=================================
// 
// Vulkan Device
// 
//=================================
struct DeviceQueueFamilyProperties_t
{
	int		graphicsIndex = -1;
	int		computeIndex = -1;
	int		transferIndex = -1;
	int		presentIndex = -1;

	bool IsComplete()
	{
		return (graphicsIndex != -1) && (presentIndex != -1) && (computeIndex != -1) && (transferIndex != -1);
	}
};


class VulkanDevice
{
public:
	VulkanDevice() 
		: m_PhysicalDeviceHandle{ VK_NULL_HANDLE }
		, m_LogicalDeviceHandle{ VK_NULL_HANDLE } {}

	bool	CreateVulkanDevice(CVulkanRenderer* renderingContext);
	void	DestroyLogicalDevice(CVulkanRenderer* renderingContext);

	VkDevice& GetLogicalDevice() { return m_LogicalDeviceHandle; }
	VkPhysicalDevice& GetPhysicalDevice() { return m_PhysicalDeviceHandle; }
	DeviceQueueFamilyProperties_t& GetPhysicalDeviceQueueIndices() { return m_PhysicalDeviceQueueIndices; }

	void QueryDeviceSwapchainSupport(CVulkanRenderer* renderingContext, VkPhysicalDevice device, VulkanSwapchain& swapchain);

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
	bool m_bDeviceSupportsSwapchain = false;
};


//=================================
// 
// Swapchain
// 
//=================================
struct SwapchainSupportDetails_t
{
	VkSurfaceCapabilitiesKHR capabilities;
	CVector<VkSurfaceFormatKHR> formats;
	CVector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
	bool		CreateSwapchain(CVulkanRenderer* renderingContext);
	void		CleanupSwapchain(CVulkanRenderer* renderingContext);

	//void		SetSwapchainSupportDetails(SwapchainSupportDetails_t* details) { m_SwapchainSupportDetails = details; }
	SwapchainSupportDetails_t& GetSwapchainSupportDetails() { return m_SwapchainSupportDetails; }

private:
	// Utility functions for choosing formats on swapchain creation.
	VkSurfaceFormatKHR	ChooseSwapchainSurfaceFormat(CVector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR	ChooseSwapchainPresentMode(CVector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D			ChooseSwapchainExtent(CVulkanRenderer* renderingContext, const VkSurfaceCapabilitiesKHR& capabilities);


private:
	VkSwapchainKHR	m_SwapchainHandle;
	SwapchainSupportDetails_t m_SwapchainSupportDetails;
};


//=================================
// 
// Vulkan Rendering Context
// 
//=================================
class CVulkanRenderer : public IRendererBackend
{
public:
	bool	RendererBackend_Initialize() override;
	void	RendererBackend_Shutdown() override;
	bool	RendererBackend_Update() override;

private:
	bool	CreateVulkanInstance();
	bool	CreateDebugMessenger();

	void	DestroyDebugMessenger();

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

public:
	VkInstance& GetVulkanInstance() { return m_Instance; }
	CVector<const char*>& GetValidationLayers() { return m_ValidationLayers; }
	// Only enabling Valdiation Layers on debug builds for now
#if _DEBUG
	bool	EnableValidationLayers() const { return true; }
#else
	bool	EnableValidationLayers() const { return false; }
#endif

	VkSurfaceKHR&	GetVulkanSurface() { return m_Surface; }
	VulkanDevice&	GetDevice() { return m_Device; }
	VulkanSwapchain& GetSwapchain() { return m_Swapchain; }

private:

	bool	RequestValidationLayerSupport();

	CVector<const char*> m_ValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
};

static CVulkanRenderer VulkanRenderingContext;

IRendererBackend* GetVulkanBackend();

