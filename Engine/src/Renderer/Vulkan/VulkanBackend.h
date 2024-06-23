//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"

#include <vulkan/vulkan.h>

// Forward Declarations
template <typename T> class CVector;

class VulkanDevice;
class CVulkanRenderer;

// Utility Macro for checking success with VkResult
#define VK_CHECK(expr) expr == VK_SUCCESS

//=================================
// 
// Vulkan Device
// 
//=================================
class VulkanDevice
{
public:
	static bool CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice);

private:
	bool		SelectPhysicalDevice(CVulkanRenderer* pRenderingContext);

private:
	VkPhysicalDevice	m_PhysicalDevice{ VK_NULL_HANDLE };

	VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};
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

	VkInstance& GetInstance() { return m_Instance; }

private:
	bool		CreateVulkanInstance();
	bool		QueryInstanceExtensionSupport(CVector<const char*>& requiredExtensions);
	bool		QueryValidationLayerSupport(CVector<const char*>& requiredValidationLayers);

	bool		CreateDebugMessenger();
	void		DestroyDebugMessenger();

private:
	// Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// TODO: Implement
	VkAllocationCallbacks* m_Allocator{ nullptr };

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };

	// Device
	VulkanDevice m_Device;

	bool		EnableValidationLayers()
	{
#ifdef _DEBUG
		return true;
#else
		return false;
#endif
	}
};

IRendererBackend* GetVulkanBackend();

