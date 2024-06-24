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
class	CVulkanRenderer;

// Utility Macro for checking success with VkResult
#define VK_CHECK(expr) expr == VK_SUCCESS

// Maximum frames that can be worked with at once
const char MAX_FRAMES_IN_FLIGHT{ 3 };

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

	bool		GetSwapchainImages(CVulkanRenderer* pRenderingContext);
	bool		CreateImageViews(CVulkanRenderer* pRenderingContext);

	VkSwapchainKHR			m_SwapchainHandle{ VK_NULL_HANDLE };
	SwapchainSupportDetails m_SwapchainSupport{};
	CVector<VkImage>		m_SwapchainImages;
	CVector<VkImageView>	m_SwapchainImageViews;

	VkFormat			m_SwapchainImageFormat;
	VkExtent2D			m_SwapchainExtent;
};

//=================================
// 
// Vulkan Command Buffer
// 
//=================================
class VulkanCommandBuffer
{
public:

	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
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

	bool			Begin(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);
	bool			End(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);

	VkRenderPass m_Handle{ VK_NULL_HANDLE };
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
	bool		RendererBackend_Update() { return true; }

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

	unsigned int m_FramebufferWidth{ 0 };
	unsigned int m_FramebufferHeight{ 0 };
};

IRendererBackend* GetVulkanBackend();

