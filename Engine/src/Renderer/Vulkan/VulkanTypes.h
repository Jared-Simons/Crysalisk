//=============================================================================
// Author: Jared Simons
// Date Created: 11/6/2024
// 
// Purpose: Vulkan Type Definitions
//=============================================================================
#pragma once

#include "Defines.h"

#include "Core/Assertions.h"
#include "Math/MatrixTypes.h"

#include <Containers/CVector.h>

#include <vulkan/vulkan.h>

// Forward declarations
class CVulkanRenderer;

// Utility macro for checking success with VkResult
// TODO: Implement!!
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

	CVector<VkFramebuffer>&		GetFramebuffers() { return m_Framebuffers; }

	VkFormat					GetImageFormat() { return m_SwapchainImageFormat; }
	VkExtent2D					GetExtent() { return m_SwapchainExtent; }

private:
	VkSwapchainKHR			m_SwapchainHandle{ VK_NULL_HANDLE };
	SwapchainSupportDetails m_SwapchainSupport{};
	CVector<VkImage>		m_SwapchainImages;
	CVector<VkImageView>	m_SwapchainImageViews;

	// Framebuffers for rendering to the screen
	CVector<VkFramebuffer> m_Framebuffers;

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
	// TODO: Is this being set/used?
	bool						m_bIsPrimary{ false };
};

//=================================
// 
// Vulkan Buffer
// 
//=================================
class VulkanBuffer
{
public:
	static bool		CreateBuffer(CVulkanRenderer* pRenderingContext, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer* pOutBuffer);
	static void		DestroyBuffer(CVulkanRenderer* pRenderingContext, VulkanBuffer* pBuffer);

	// TODO: Handle offsets and other data here
	void			UploadDataToBuffer(CVulkanRenderer* pRenderingContext, u64 dataSize, void* data, u64 offset) const;
	bool			CopyBufferToImage(CVulkanRenderer* pRenderingContext, u64 width, u64 height, class VulkanImage* pImage) const;

	inline VkBuffer&		GetHandle() { return m_Handle; }
	inline VkDeviceMemory&	GetMemory() { return m_Memory; }

private:

	VkBuffer		m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory	m_Memory{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Image
// 
//=================================
class VulkanImage
{
public:
	static bool		CreateImage(CVulkanRenderer* pRenderingContext, u64 width, u64 height, i32 numChannels, void* pPixelData, bool bCreateView, VulkanImage* pOutImage);
	static void		DestroyImage(CVulkanRenderer* pRenderingContext, VulkanImage* pImage);

	bool			TransitionImageLayout(CVulkanRenderer* pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout) const;

	VkImage&		GetHandle() { return m_Handle; }
	VkDeviceMemory& GetMemory() { return m_ImageMemory; }
	VkImageView&	GetImageView() { return m_ImageView; }
	VkSampler&		GetSampler() { return m_ImageSampler; }
	VkFormat		GetFormat() { return m_ImageFormat; }

private:
	VkImage			m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory	m_ImageMemory{ VK_NULL_HANDLE };
	VkImageView		m_ImageView{ VK_NULL_HANDLE };
	VkSampler		m_ImageSampler{ VK_NULL_HANDLE };

	VkFormat		m_ImageFormat{ VK_FORMAT_UNDEFINED };
};

const u32 VULKAN_MAX_GLOBAL_TEXTURES = 32;
const u32 VULKAN_MAX_INSTANCE_COUNT = 1024;
const u32 VULKAN_SHADER_SAMPLER_COUNT = 1;

//=================================
// 
// Uniform Buffer
// 
//=================================
// TODO: Move to the frontend
struct UniformBufferObject
{
	UniformBufferObject() {}

	Matrix4x4 model;
	Matrix4x4 view;
	Matrix4x4 projection;
};

struct VulkanDescriptorSetState
{
	u8 generation[3];
	VkDescriptorSet descriptorSets[3];
};


//=================================
// 
// Vulkan Shader
// 
//=================================
class VulkanShader
{
public:
	VulkanShader() {}

	static bool		CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);
	static void		DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader);

	static bool		CreatePipeline(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);
	bool			CreateDescriptorResources(CVulkanRenderer* pRenderingContext);

	bool			Use(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer, u32 imageIndex) const;
	void			UpdateUniforms(CVulkanRenderer* pRenderingContext, u32 imageIndex, struct Camera s_Camera, f32 deltaTime);
	bool			AcquireInstanceResources(CVulkanRenderer* pRenderingContext, class Texture& pTexture);

	void			BindGlobals(VkCommandBuffer commandBuffer, u32 imageIndex);
	void			ApplyGlobals(CVulkanRenderer* pRenderingContext, VkCommandBuffer commandBuffer, u32 imageIndex, Matrix4x4 projection, Matrix4x4 view);

	void			BindInstance(VkCommandBuffer commandBuffer, u32 imageIndex, class Texture& pTexture);
	void			ApplyInstance(CVulkanRenderer* pRenderingContext, VkCommandBuffer commandBuffer, u32 imageIndex, class Texture& pTexture);
	void			ApplyModel(VkCommandBuffer commandBuffer, Matrix4x4 model) const;

private:

public:
	VkShaderModule		m_VertModule{ VK_NULL_HANDLE };
	VkShaderModule		m_FragModule{ VK_NULL_HANDLE };

	VkPipeline			m_Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout	m_PipelineLayout{ VK_NULL_HANDLE };

	VkDescriptorPool			m_GlobalDescriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSetLayout		m_GlobalDescriptorSetLayout{ VK_NULL_HANDLE };
	VulkanDescriptorSetState	m_GlobalDescriptorSets{};

	VkDescriptorPool			m_InstanceDescriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSetLayout		m_InstanceDescriptorSetLayout{ VK_NULL_HANDLE };
	VulkanDescriptorSetState	m_InstanceDescriptorSets[VULKAN_MAX_INSTANCE_COUNT]{}; // * IMAGE_SAMPLER_COUNT?

	VulkanBuffer				m_UniformBuffer{};
	void* m_UniformBufferMappedMemory{};

	u32							m_InstanceDescriptorOffset{ 0 };
	u32							m_CurrentlyBoundInstance{ INVALID_ID_U8 };

	// NOTE: This only exists as a placeholder for now.
	VulkanImage m_GlobalTexture{};
};