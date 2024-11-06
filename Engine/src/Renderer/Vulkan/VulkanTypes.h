//=============================================================================
// Author: Jared Simons
// Date Created: 11/6/2024
// 
// Purpose: Vulkan Type Definitions
//=============================================================================
#pragma once

#include "Defines.h"

#include "Core/Assertions.h"

#include <Containers/CVector.h>

#include <vulkan/vulkan.h>

// Forward declarations
class CVulkanRenderer;

template<typename T> class CVector;

// Utility macro for checking success with VkResult
/*#define VK_CHECK(expr)		\
	{						\
		if (!expr)			\
			{				\
				CAssert();	\
			}				\
	}*/

//=================================
// 
// Vulkan Device
// 
//=================================
struct DeviceQueueFamilyInfo_t
{
	u8 GraphicsQueueIndex{ INVALID_ID_U8 };
	u8 ComputeQueueIndex{ INVALID_ID_U8 };
	u8 TransferQueueIndex{ INVALID_ID_U8 };
	u8 PresentQueueIndex{ INVALID_ID_U8 };

	bool IsConceptOccupied(u8* queueIndex) { return *queueIndex != INVALID_ID_U8; }
	bool IsIndexOccupied(u8 index) const
	{
		return (GraphicsQueueIndex == index) || (ComputeQueueIndex == index) ||
			(TransferQueueIndex == index) || (PresentQueueIndex == index);
	}

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

	u64 GetNumberOfValidQueues()
	{
		u64 numQueues{ 0 };

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

//=================================
// 
// Vulkan Device
// 
//=================================
class VulkanDevice
{
public:
	static bool			CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice);
	static void			DestroyDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pDevice);

	u32					FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags& properties) const;

	void				QuerySwapchainSupportDetails(CVulkanRenderer* pRenderingContext, struct SwapchainSupportDetails* pOutDetails) const;

	inline VkDevice		GetLogicalDevice() { return m_LogicalDevice; }

	inline VkQueue		GetGraphicsQueue() { return m_GraphicsQueue; }
	inline VkQueue		GetPresentQueue() { return m_PresentQueue; }
	inline VkQueue		GetTransferQueue() { return m_TransferQueue; }
	inline VkQueue		GetComputeQueue() { return m_ComputeQueue; }

	inline VkPhysicalDeviceProperties	GetPhysicalDeviceProperties() { return m_PhysicalDeviceProperties; }
	inline DeviceQueueFamilyInfo_t		GetPhysicalDeviceQueueFamilyInfo() { return m_PhysicalDeviceQueueFamilyInfo; }

private:
	bool				SelectPhysicalDevice(CVulkanRenderer* pRenderingContext);
	bool				QueryDeviceQueueFamilyProperties(CVulkanRenderer* pRenderingContext, VkPhysicalDevice& physicalDevice, DeviceQueueFamilyInfo_t* pOutQueueInfo);
	bool				QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice);

	bool				CreateLogicalDevice(CVulkanRenderer* pRenderingContext);

private:
	VkDevice			m_LogicalDevice{ VK_NULL_HANDLE };

	VkQueue				m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue				m_PresentQueue{ VK_NULL_HANDLE };
	VkQueue				m_TransferQueue{ VK_NULL_HANDLE };
	VkQueue				m_ComputeQueue{ VK_NULL_HANDLE };

	VkPhysicalDevice	m_PhysicalDevice{ VK_NULL_HANDLE };

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
	CVector<VkSurfaceFormatKHR>	formats;
	CVector<VkPresentModeKHR>	presentModes;
};

class VulkanSwapchain
{
public:
	static bool				CreateSwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pOutSwapchain);
	static void				DestroySwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain);

	static bool				CreateFramebuffers(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain);

	bool					RecreateSwapchain(CVulkanRenderer* pRenderingContext, u32 newWidth, u32 newHeight);
	bool					Present(CVulkanRenderer* pRenderingContext, u32 imageIndex);

	bool					AcquireSwapchainImages(CVulkanRenderer* pRenderingContext);
	bool					CreateImageViews(CVulkanRenderer* pRenderingContext);

	VkSwapchainKHR&				GetHandle() { return m_SwapchainHandle; }
	SwapchainSupportDetails&	GetSupportDetails() { return m_SwapchainSupport; }

	CVector<VkImageView>&		GetImageViews() { return m_SwapchainImageViews; }

	CVector<class VulkanFramebuffer> GetFramebuffers() { return m_Framebuffers; }

	VkFormat					GetImageFormat() { return m_SwapchainImageFormat; }
	VkExtent2D					GetExtent() { return m_SwapchainExtent; }

private:
	VkSwapchainKHR			m_SwapchainHandle{ VK_NULL_HANDLE };
	SwapchainSupportDetails m_SwapchainSupport{};
	CVector<VkImage>		m_SwapchainImages;
	CVector<VkImageView>	m_SwapchainImageViews;

	// Framebuffers for rendering to the screen
	CVector<class VulkanFramebuffer> m_Framebuffers;

	VkFormat				m_SwapchainImageFormat;
	VkExtent2D				m_SwapchainExtent;
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

	bool			Begin(CVulkanRenderer* pRenderingContext, class VulkanCommandBuffer* pCommandBuffer, u32 imageIndex);
	void			End(CVulkanRenderer* pRenderingContext, class VulkanCommandBuffer* pCommandBuffer);

	VkRenderPass	GetHandle() { return m_Handle; }

private:
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
	INITIAL,
	/* The command buffer can be written to with vkCmd calls. */
	RECORDING,
	/* The command buffer has been written to and can be freed or submitted for execution. */
	EXECUTABLE,
	/* The command buffer has been submitted for queue submission and CANNOT be modified at this point. */
	PENDING,
	/* The command buffer has been invalidated through an operation that modified it's resources, can be freed or rest. */
	INVALID
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

	bool			BeginSingleUse(CVulkanRenderer* pRenderingContext, VkCommandPool* pCommandPool);
	void			EndSingleUse(CVulkanRenderer* pRenderingContext, VkCommandPool* pCommandPool);

	VkCommandBuffer&			GetHandle() { return m_Handle; }
	eVulkanCommandBufferState	GetState() { return m_State; }
	bool						IsPrimaryBuffer() { return m_bIsPrimary; }

	void						SetState(eVulkanCommandBufferState state) { m_State = state; }

private:
	VkCommandBuffer				m_Handle{ VK_NULL_HANDLE };
	eVulkanCommandBufferState	m_State{ eVulkanCommandBufferState::INITIAL };

	// Is this a primary or secondary level command buffer?
	// TODO: Is this being set?
	bool						m_bIsPrimary{ false };
};