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
class VulkanDevice_t;


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
};


class VulkanDevice_t
{
public:
	VulkanDevice_t() : m_PhysicalDeviceHandle{ VK_NULL_HANDLE } {}

	bool	CreateVulkanDevice(CVulkanRenderer* renderingContext);
	void	DestroyLogicalDevice(CVulkanRenderer* renderingContext);

	VkDevice& GetLogicalDevice() { return m_LogicalDeviceHandle; }

private:
	bool	SelectPhysicalDevice(CVulkanRenderer* renderingContext);
	bool	PhysicalDeviceMeetsRequirements(VkPhysicalDevice& physicalDevice);
	void	GetQueueFamilyData();

	bool	CreateLogicalDevice(CVulkanRenderer* renderingContext);

private:
	VkPhysicalDevice m_PhysicalDeviceHandle;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
	DeviceQueueFamilyProperties_t m_PhysicalDeviceQueueFamilyProperties;

	VkDevice m_LogicalDeviceHandle;
	VkQueue m_GraphicsQueue;
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

	// Vulkan Device
	VulkanDevice_t	m_Device;

public:
	VkInstance& GetVulkanInstance() { return m_Instance; }
	CVector<const char*>& GetValidationLayers() { return m_ValidationLayers; }
	// Only enabling Valdiation Layers on debug builds for now
#if _DEBUG
	bool	EnableValidationLayers() const { return true; }
#else
	bool	EnableValidationLayers() const { return false; }
#endif

private:

	bool	RequestValidationLayerSupport();

	CVector<const char*> m_ValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
};

static CVulkanRenderer VulkanRenderingContext;

IRendererBackend* GetVulkanBackend();

