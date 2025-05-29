//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include <Renderer/IRendererBackend.h>

#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>

#include "VulkanTypes.h"

#include <Containers/CVector.h>
#include <Resources/Loaders/ShaderLoader.h> // TODO: Move types to their own files!

class CVulkanRenderer : public IRendererBackend
{
public:
	virtual bool	RendererBackend_Initialize(RendererConfig_t& pConfig) override;
	virtual void	RendererBackend_Shutdown() override;

	virtual bool	StartFrame(struct RenderPacket_t& pRenderData) override;
	virtual void	EndFrame(struct RenderPacket_t& pRenderData) override;

	virtual bool	StartRenderpass(RendererView& pRendererView) override;
	virtual void	EndRenderpass() override;

	virtual void	DrawGeometry(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, struct GeometryInstance_t& pGeometry, struct MaterialReference& pMaterial) override;
	virtual void	RenderTestUI(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry) override;

	virtual bool	CreateRenderBufferInternal(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, RenderBuffer& pOutBuffer) override;
	virtual void	DestroyRenderBufferInternal(RenderBuffer& pRenderBuffer) override;

	virtual bool	CreateRendererViewInternal(RendererView& pOutView) override;
	virtual void	DestroyRendererViewInternal(RendererView& pView) override;

	virtual bool	AcquireTextureResources(class Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) override;
	virtual void	ReleaseTextureResources(class Texture* pTexture) override;

	virtual bool	AcquireTextureResources_OLD(class Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) override;
	virtual void	ReleaseTextureResources_OLD(class Texture& pTexture) override;

	virtual	bool	AcquireMaterialResources(class Material& pMaterial, struct MaterialInstanceData_t& pMaterialInstanceData) override; // TODO: Pass in shader as well

	virtual bool	CreateShader(class Shader* pShader, struct ShaderConfig_t& pConfig) override;
	virtual void	DestroyShader(class Shader* pShader) override;

	virtual void	AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID) override;

	virtual void	BindShader(class Shader* pShader) override;

	virtual void	BindPerFrameShaderDescriptors(class Shader* pShader) override;
	virtual void	BindPerInstanceShaderDescriptors(class Shader* pShader, unsigned int hInstanceID) override;

	virtual void	SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) override;

	virtual bool	AcquireShaderResources_OLD(class Shader& pShader, struct ShaderConfig_t& pShaderConfig) override;
	virtual void	ReleaseShaderResources(class Shader& pShader) override;

	virtual void	BindShader_OLD(class Shader& pShader) override;

	virtual bool	AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance) override;
	virtual bool	AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance) override;
	virtual void	ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry) override;

	// NOTE: These are also temporary.
	void DestroyDefaultGeometry();

	// All this stuff can be public because nothing outside of this module should be accessing the backend...
	// Any outside contact with the Vulkan Renderer should go through an IRenderer interface.
public:
	// Vulkan Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// Memory Allocator Hooks - TODO: IMPLEMENT
	VkAllocationCallbacks* m_Allocator{ nullptr };

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

	// UI Renderpass
	VulkanRenderPass m_UIRenderPass{};

	// Command Buffers
	VkCommandPool	m_GraphicsCommandPool{ VK_NULL_HANDLE };
	Utl::CVector<VulkanCommandBuffer> m_GraphicsCommandBuffers{};

	// Sync Objects
	VkSemaphore	m_ImageAvailableSemaphore{ VK_NULL_HANDLE };
	VkSemaphore m_RenderFinishedSemaphore{ VK_NULL_HANDLE };
	VkFence		m_InFlightFence{ VK_NULL_HANDLE };

	// Descriptor Pool
	VkDescriptorPool	m_DescriptorPool{ VK_NULL_HANDLE };

	// Internal Vulkan texture tata
	VulkanTextureInternal* m_pInternalTextureData{ nullptr };

	// Internal shader data
	VulkanShaderInternal* m_pInternalShaderData{ nullptr };
	unsigned int			m_CurrentlyBoundShaderIndex{ kInvalidID_U32 };
	VulkanShaderInternal* m_pCurrentlyBoundShader{ nullptr };

	// The number of frames that can be processed at once.
	unsigned int m_MaxImagesInFlight{ 3 };
	bool		m_bCurrentlyRendering[3]{ false };
	int			m_RenderMode{ 0 };
	double		m_flDeltaTime{ 0.0f };
	double		m_flLastFrameTime{ 0.0f };
	unsigned int	m_ImageIndex{ 0 };
	size_t		m_CurrentRendererFrame{ 0 };
	bool		m_bWindowResized{ false };
	bool		m_bResizingFramebuffer{ false };
	RendererConfig_t	m_RendererConfig{};
};

extern IRendererBackend* g_pRendererBackend;

// Some platform specific utilities.
namespace Platform 
{
	const char* GetVulkanPlatformExtension();
	bool CreateVulkanSurface(CVulkanRenderer& pRenderingContext, void* pHWnd);
}