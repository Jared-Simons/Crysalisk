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

#define VK_CHECK(expr)		\
	if (expr != VK_SUCCESS)	\
		CAssert(0);			\

const int kMaxVulkanInstances = 2048;

class VulkanDevice
{
public:
	static bool	CreateDevice(class CVulkanRenderer& pRenderingContext, VulkanDevice& pOutDevice);
	static void	DestroyDevice(class CVulkanRenderer& pRenderingContext, VulkanDevice& pDevice);

	uint32_t	FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat	FindSupportedDepthFormat();

public:
	VkPhysicalDevice	m_PhysicalDevice{ VK_NULL_HANDLE };
	VkDevice			m_Device{ VK_NULL_HANDLE };

	VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
	VkPhysicalDeviceFeatures		m_DeviceFeatures{};
	VkPhysicalDeviceProperties		m_DeviceProperties{};

	struct DeviceQueueFamilyIndices
	{
		int	GraphicsQueueFamilyIndex{ -1 };
		int PresentQueueFamilyIndex{ -1 };
	};

	DeviceQueueFamilyIndices	m_QueueFamilyIndices{};

	VkQueue				m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue				m_PresentQueue{ VK_NULL_HANDLE };
};

class VulkanImage
{
public:
	static bool CreateImage(class CVulkanRenderer& pRenderingContext, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VulkanImage& pOutImage);
	static void DestroyImage(class CVulkanRenderer& pRenderingContext, VulkanImage& pImage);

	bool		TransitionImageLayout(class CVulkanRenderer& pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout);

	bool		WriteDataToImage(class CVulkanRenderer& pRenderingContext, uint32_t width, uint32_t height, uint8_t numChannels, byte* pPixelData);

public:
	VkImage			m_Image{ VK_NULL_HANDLE };
	VkDeviceMemory	m_Memory{ VK_NULL_HANDLE };
	VkImageView		m_View{ VK_NULL_HANDLE };
};

class VulkanBuffer
{
public:
	static bool		CreateBuffer(class CVulkanRenderer& pRenderingContext, size_t bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, bool bUseFreelist, VulkanBuffer& pOutBuffer);
	static void		DestroyBuffer(class CVulkanRenderer& pRenderingContext, VulkanBuffer& pBuffer);

	size_t			UploadDataRange(class CVulkanRenderer& pRenderingContext, size_t dataSize, void* pData);
	void			FreeDataRange(class CVulkanRenderer& pRenderingContext, size_t dataOffset, size_t dataSize);

public:
	VkBuffer		m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory	m_Memory{ VK_NULL_HANDLE };
	size_t	m_BufferSize{ 0 };
	size_t	m_BytesAllocated{ 0 };
	bool			m_bUsesFreelist{ false };
	Utl::CFreeList	m_Freelist{};
	void* m_pFreelistMemory{ nullptr };
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
	static bool		CreateSwapchain(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pOutSwapchain);
	static void		DestroySwapchain(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain);
	static bool		CreateFramebuffers(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain);
	static void		RecreateSwapchain(class CVulkanRenderer& pRenderingContext, VulkanSwapchain& pSwapchain);

public:
	VkSwapchainKHR	m_Handle{ VK_NULL_HANDLE };

	VkFormat		m_SwapchainImageFormat{};
	VkFormat		m_DepthImageFormat{};
	VkExtent2D		m_SwapchainExtent{};

	Utl::CVector<VkImage> m_SwapchainImages{};
	Utl::CVector<VkImageView> m_SwapchainImageViews{};
	Utl::CVector<VkFramebuffer> m_Framebuffers{};
	class VulkanImage m_DepthImage{};
};

class VulkanRenderPass
{
public:
	static bool CreateRenderpass(class CVulkanRenderer& pRenderingContext, bool bShouldClear, VulkanRenderPass& pOutRenderpass);
	static void DestroyRenderpass(class CVulkanRenderer& pRenderingContext, VulkanRenderPass& pRenderpass);

public:
	VkRenderPass	m_Handle{ VK_NULL_HANDLE };
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
		byte			m_Generation;
	};

	size_t		m_Generation{ kInvalidID_U32 };
	VkPipeline	m_Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

	VkDescriptorPool		m_DescriptorPool{};
	VkDescriptorSetLayout	m_GlobalDescriptorLayout{};
	VkDescriptorSetLayout	m_InstanceDescriptorLayout{};
	Utl::CVector<VulkanDescriptorSetLayoutState> m_DescriptorSetLayouts{};
	Utl::CVector<VulkanDescriptorSetState>	m_DescriptorSets{};
	int						m_NextFreeDescriptorSetIndex{ 0 };
	size_t					m_CurrentRendererFrame{ 0 };

	Utl::CVector<VkSampler>		m_InstanceSamplers{};
	Utl::CVector<VulkanBuffer>	m_UniformBuffers{};
	GlobalUniformObject_t		m_UniformData{};
};

struct VulkanTextureInternal
{
	class VulkanImage	m_Image{};
	unsigned char		m_Generation{ kInvalidID_U8 };
};
