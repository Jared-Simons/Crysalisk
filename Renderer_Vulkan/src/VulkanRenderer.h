//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include <Renderer/IRendererBackend.h>

#include <Core/Clock.h>
#include <Renderer/RendererTypes.h>

#include "VulkanTypes.h"

#include <Containers/CVector.h>

class CVulkanRenderer : public IRendererBackend
{
public:
	virtual bool	RendererBackend_Initialize(RendererConfig_t& pConfig) override;
	virtual void	RendererBackend_Shutdown() override;
	virtual bool	StartFrame(struct RenderPacket_t& pRenderData) override;
	virtual void	EndFrame(struct RenderPacket_t& pRenderData) override;

	virtual void	DrawGeometry(struct GeometryInstance_t& pGeometry) override;

	virtual bool	AcquireTextureResources(class Texture& pTexture, size_t nPixelDataSize, byte* pPixelData) override;
	virtual void	ReleaseTextureResources(class Texture& pTexture) override;

	virtual bool	AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance) override;

	// NOTE: These methods are temporary until we implement a shader system on the front-end to handle this.
	bool CreateDefaultShader(VulkanShaderInternal& pOutShader);
	void DestroyDefaultShader(VulkanShaderInternal& pShader);

	// NOTE: These are also temporary.
	void DestroyDefaultGeometry();

// All this stuff can be public because nothing outside of this module should be accessing the backend...
// Any outside contact with the Vulkan Renderer should go through an IRenderer interface.
public:
	// Vulkan Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// Memory Allocator Hooks - TODO: IMPLEMENT
	VkAllocationCallbacks m_Allocator{ nullptr };

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };

	// Surface
	VkSurfaceKHR	m_Surface{ VK_NULL_HANDLE };

	// Device
	VulkanDevice m_Device{};

	// Swapchain
	VulkanSwapchain m_Swapchain{};

	// Main Renderpass
	VulkanRenderPass m_MainRenderPass{};

	// Command Buffers
	VkCommandPool	m_GraphicsCommandPool{ VK_NULL_HANDLE };
	Utl::CVector<VulkanCommandBuffer> m_GraphicsCommandBuffers{};

	// Sync Objects
	VkSemaphore	m_ImageAvailableSemaphore{ VK_NULL_HANDLE };
	VkSemaphore m_RenderFinishedSemaphore{ VK_NULL_HANDLE };
	VkFence		m_InFlightFence{ VK_NULL_HANDLE };

	// Temp shader
	VulkanShaderInternal m_DefaultShader{};
	VulkanTextureInternal m_TestTexture{};

	// Descriptor Pool
	VkDescriptorPool	m_DescriptorPool{ VK_NULL_HANDLE };

	// Temp geometry buffer
	VulkanBuffer		m_GeometryVertexBuffer{};
	VulkanBuffer		m_GeometryIndexBuffer{};
	Utl::CVector<VulkanBuffer> m_UniformBuffers{};
	GlobalUniformObject_t	m_UniformData{};

	// The number of frames that can be processed at once.
	unsigned int m_MaxImagesInFlight{ 3 };
	double		m_flDeltaTime{ 0.0f };
	double		m_flLastFrameTime{ 0.0f };
	unsigned int	m_ImageIndex{ 0 };
	bool		m_bWindowResized{ false };
	bool		m_bResizingFramebuffer{ false };
	Clock		m_RendererClock{};
};

extern IRendererBackend* g_pRendererBackend;

// Some platform specific utilities.
namespace Platform {
	const char* GetVulkanPlatformExtension();
	bool CreateVulkanSurface(CVulkanRenderer& pRenderingContext, void* pHWnd);
}