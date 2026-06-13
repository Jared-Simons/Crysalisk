//=============================================================================
// Author: Jared Simons
// Date Created: 2/10/2025
// 
// Purpose: Includes Vulkan Type Definitions used by the renderer.
//=============================================================================
#pragma once

#include <vulkan/vulkan.h>

#include <Containers/CVector.h>
#include <Core/Assertions.h>
#include <Renderer/RendererTypes.h>
#include "VulkanUtils.h"

#ifdef _DEBUG
#define VK_CHECK(expr)		\
	if (expr != VK_SUCCESS)	\
		CAssert(0);			
#else
#define VK_CHECK(expr)		\
	expr
#endif

const i32 kMaxGlobalTextures = 32;
const i32 kMaxVulkanInstances = 2048;

class VulkanDevice
{
public:
	static bool	CreateDevice(class CVulkanRenderer& pRenderingContext, VulkanDevice& pOutDevice);
	static void	DestroyDevice(class CVulkanRenderer& pRenderingContext, VulkanDevice& pDevice);

	u32	FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	VkFormat	FindSupportedDepthFormat();

public:
	VkPhysicalDevice	m_PhysicalDevice{ VK_NULL_HANDLE };
	VkDevice			m_Device{ VK_NULL_HANDLE };

	VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
	VkPhysicalDeviceFeatures		m_DeviceFeatures{};
	VkPhysicalDeviceProperties		m_DeviceProperties{};

	struct DeviceQueueFamilyIndices
	{
		i32	GraphicsQueueFamilyIndex{ -1 };
		i32 PresentQueueFamilyIndex{ -1 };
	};

	DeviceQueueFamilyIndices	m_QueueFamilyIndices{};

	VkQueue				m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue				m_PresentQueue{ VK_NULL_HANDLE };
};

class VulkanImage
{
public:
	static bool CreateImage(class CVulkanRenderer& pRenderingContext, const char* pszImageName, u32 width, u32 height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VulkanImage& pOutImage);
	static void DestroyImage(class CVulkanRenderer& pRenderingContext, VulkanImage& pImage);

	bool		TransitionImageLayout(class CVulkanRenderer& pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout);

	bool		WriteDataToImage(class CVulkanRenderer& pRenderingContext, u32 width, u32 height, u8 numChannels, byte* pPixelData);
	void		CopyBufferToImage(class CVulkanRenderer& pRenderingContext, class VulkanBuffer& pStagingBuffer, u32 width, u32 height);

public:
	VkImage			m_Image{ VK_NULL_HANDLE };
	VkDeviceMemory	m_Memory{ VK_NULL_HANDLE };
	VkImageView		m_View{ VK_NULL_HANDLE };
};

class VulkanBuffer
{
public:
	static bool		CreateBuffer(class CVulkanRenderer& pRenderingContext, u64 bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, bool bUseFreelist, VulkanBuffer& pOutBuffer);
	static void		DestroyBuffer(class CVulkanRenderer& pRenderingContext, VulkanBuffer& pBuffer);
	static void		Resize(class CVulkanRenderer& pRenderingContext, VulkanBuffer* pBuffer, u64 iNewBufferSize);

	u64			UploadDataRange(class CVulkanRenderer& pRenderingContext, u64 dataSize, void* pData);
	void			FreeDataRange(class CVulkanRenderer& pRenderingContext, u64 dataOffset, u64 dataSize);

public:
	VkBuffer				m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory			m_Memory{ VK_NULL_HANDLE };
	VkMemoryPropertyFlags	m_MemoryProperties{};
	VkBufferUsageFlags		m_BufferUsage{};

	// FIXME: MOVE THESE TO THE RENDERBUFFER CLASS!
	u64					m_BufferSize{ 0 };
	u64					m_BytesAllocated{ 0 };
	
	bool			m_bUsesFreelist{ false };
	Utl::CFreeList	m_Freelist{};
	void*			m_pFreelistMemory{ nullptr };
};

class VulkanCommandBuffer
{
public:
	static bool		CreateCommandBuffer(class CVulkanRenderer& pRenderingContext, bool bIsPrimary, VulkanCommandBuffer& pOutCommandBuffer);
	static void		DestroyCommandBuffer(class CVulkanRenderer& pRenderingContext, VulkanCommandBuffer& pCommandBuffer);

	bool			CreateAndBeginSingleUse(class CVulkanRenderer& pRenderingContext, VkCommandPool pCommandPool);
	void			EndSingleUse(class CVulkanRenderer& pRenderingContext, VkCommandPool pCommandPool);

public:
	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
};

class VulkanSwapchain
{
public:
	static bool		CreateSwapchain(class CVulkanRenderer& pRenderingContext, VkSurfaceKHR pSurface, VulkanSwapchain& pOutSwapchain);
	static void		DestroySwapchain(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain);
	static bool		CreateFramebuffers(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain, class VulkanRenderPass* pMainRenderpass);
	static void		RecreateSwapchain(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain);

public:
	VkSwapchainKHR	m_Handle{ VK_NULL_HANDLE };

	VkFormat		m_SwapchainImageFormat{};
	VkFormat		m_DepthImageFormat{};
	VkExtent2D		m_SwapchainExtent{};

	Utl::CVector<VkImage>		m_SwapchainImages{};
	Utl::CVector<VkImageView>	m_SwapchainImageViews{};
	Utl::CVector<VkFramebuffer> m_Framebuffers{};
	class VulkanImage			m_DepthImage{};
	class VulkanRenderPass*		m_pFramebufferPass{ nullptr };
};

struct VulkanWindowState_t
{
	VkSurfaceKHR	m_WindowSurface{};
	VulkanSwapchain	m_Swapchain{};
};

class VulkanRenderPass
{
public:
	VkRenderPass	m_Handle{ VK_NULL_HANDLE };
	char*			m_pszPassName{ nullptr };
};

struct VulkanShaderModule_t
{
	VkShaderStageFlagBits	eStage{};
	VkShaderModule			hModule{};
};

class VulkanShaderInternal
{
public:
	struct VulkanDescriptorSetLayoutState
	{
		VkDescriptorSetLayout			m_Layout;
		eShaderUniformGroup	m_FrequencyLevel;
	};

	struct VulkanDescriptorSetState
	{
		VkDescriptorSet m_DescriptorSets[3];
		byte			m_Generation[4];	// 1 Uniform Buffer, 3 Images TODO: Make dynamic
	};

	u64		m_Generation{ kInvalidID_U32 };
	VkPipeline	m_Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

	VkDescriptorPool		m_DescriptorPool{};
	VkDescriptorSetLayout	m_GlobalDescriptorLayout{};
	VkDescriptorSetLayout	m_InstanceDescriptorLayout{};
	Utl::CVector<VulkanDescriptorSetLayoutState> m_DescriptorSetLayouts{};
	Utl::CVector<VulkanDescriptorSetState>	m_DescriptorSets{};
	i32						m_NextFreeDescriptorSetIndex{ 0 };
	u64					m_CurrentRendererFrame{ 0 };

	Utl::CVector<VkSampler>		m_ImageSamplers{};
	Utl::CVector<VulkanBuffer>	m_UniformBuffers{};
};

struct VulkanTextureInternal
{
	class VulkanImage	m_Image{};
	u8		m_Generation{ kInvalidID_U8 };
};
