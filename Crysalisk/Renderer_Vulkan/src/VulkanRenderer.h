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

class CVulkanRenderer : public IRendererBackend
{
public:
	virtual bool	RendererBackend_Initialize(RendererConfig_t& pConfig) override;
	virtual void	RendererBackend_Shutdown() override;

	virtual bool	RegisterWindow(Platform::IPlatformWindow& pWindow) override;
	virtual void	ReleaseWindowResources(Platform::IPlatformWindow& pWindow) override;

	virtual bool	CreateSwapchainFramebuffers(class RendererView& pMainView) override;

	virtual bool	BeginFrame(class Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData) override;
	virtual void	EndFrame(Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData) override;

	virtual bool	CreateRenderpass(struct RenderpassConfig_t* pConfig, struct Renderpass* pRenderpass) override;
	virtual void	DestroyRenderpass(struct Renderpass* pRenderpass) override;
	virtual bool	BeginRenderpass(struct Renderpass* pRenderpass) override;
	virtual void	EndRenderpass() override;

	virtual void	DrawGeometry(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, struct GeometryInstance_t& pGeometry) override;

	virtual bool	CreateRenderBufferInternal(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer) override;
	virtual void	DestroyRenderBufferInternal(RenderBuffer& pRenderBuffer) override;

	virtual bool	AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) override;
	virtual void	ReleaseTextureResources(struct Texture* pTexture) override;

	virtual bool	CreateShader(class Shader* pShader, struct ShaderConfig_t& pConfig) override;
	virtual void	DestroyShader(class Shader* pShader) override;

	virtual void	AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName) override;
	virtual void	ReleaseShaderInstanceResources(class Shader* pShader, unsigned int pInstanceID) override;

	virtual void	BindShader(class Shader* pShader) override;

	virtual void	BindPerFrameShaderDescriptors(class Shader* pShader) override;
	virtual void	BindPerInstanceShaderDescriptors(class Shader* pShader, unsigned int hInstanceID) override;

	virtual void	SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) override;

	virtual void	AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, uint64_t iVertexBase, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) override;
	virtual void	ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry) override;

	virtual void	UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig) override;
	virtual void	UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig) override;
	virtual void	UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) override;

	// All this stuff can be public because nothing outside of this module should be accessing the backend...
	// Any outside contact with the Vulkan Renderer should go through an IRenderer interface.
public:
	// Vulkan Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// Memory Allocator Hooks - TODO: IMPLEMENT
	VkAllocationCallbacks* m_Allocator{ nullptr };

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };

	// Device
	VulkanDevice m_Device{};

	// Main application window.
	Platform::IPlatformWindow*	m_pMainWindow{ nullptr };

	// Command Buffers
	VkCommandPool	m_GraphicsCommandPool{ VK_NULL_HANDLE };
	Utl::CVector<VulkanCommandBuffer> m_GraphicsCommandBuffers{};

	// Sync Objects
	VkSemaphore	m_ImageAvailableSemaphore{ VK_NULL_HANDLE };
	VkSemaphore m_RenderFinishedSemaphore{ VK_NULL_HANDLE };
	VkFence		m_InFlightFence{ VK_NULL_HANDLE };

	// Internal Vulkan texture tata
	VulkanTextureInternal* m_pInternalTextureData{ nullptr };

	// Internal shader data
	VulkanShaderInternal* m_pInternalShaderData{ nullptr };
	unsigned int			m_CurrentlyBoundShaderIndex{ kInvalidID_U32 };
	VulkanShaderInternal* m_pCurrentlyBoundShader{ nullptr };

#ifdef VULKAN_RENDERER_RESOURCE_DEBUGGING_ENABLED
	// Debugging PFNs
	PFN_vkSetDebugUtilsObjectNameEXT	m_pfnSetDebugObjectName{ nullptr };
	PFN_vkCmdBeginDebugUtilsLabelEXT	m_pfnBeginDebugLabel{ nullptr };
	PFN_vkCmdEndDebugUtilsLabelEXT		m_pfnEndDebugLabel{ nullptr };
#endif

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

// Some platform-specific utilities.
namespace Platform 
{
	const char* GetVulkanPlatformExtension();
	bool CreateVulkanWindowSurface(CVulkanRenderer& pRenderingContext, Platform::IPlatformWindow& pWindow);
	bool CreateVulkanSurface(CVulkanRenderer& pRenderingContext, Platform::IPlatformWindow& pWindow);
}