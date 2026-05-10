//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The interface to the renderer backend
// Not to be accessed directly! Rather through the renderer frontend
//=============================================================================
#pragma once

#include "Defines.h"
#include "RendererTypes.h"

//-------------------------------------------------------------------------
// Interface for the renderer backend.
// NOTE: This should only be accessed through the IRenderer interface.
// Do not go about including "VulkanRenderer.h" outside of it's library.
//-------------------------------------------------------------------------
class IRendererBackend
{
public:
	// Initialization, shutdown
	virtual bool	RendererBackend_Initialize(struct RendererConfig_t& pAppConfig) = 0;
	virtual void	RendererBackend_Shutdown() = 0;

	virtual bool	RegisterWindow(Platform::IPlatformWindow& pWindow) = 0;
	virtual void	ReleaseWindowResources(Platform::IPlatformWindow& pWindow) = 0;

	virtual bool	CreateSwapchainFramebuffers(class RendererView& pMainView) = 0;

	virtual bool	BeginFrame(class Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData) = 0;
	virtual void	EndFrame(Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData) = 0;

	virtual bool	CreateRenderpass(struct RenderpassConfig_t* pConfig, struct Renderpass* pRenderpass) = 0;
	virtual void	DestroyRenderpass(struct Renderpass* pRenderpass) = 0;
	virtual bool	BeginRenderpass(struct Renderpass* pRenderpass) = 0;
	virtual void	EndRenderpass() = 0;

	virtual void	DrawGeometry(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, struct GeometryInstance_t& pGeometry) = 0;

	virtual bool	CreateRenderBufferInternal(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer) = 0;
	virtual void	DestroyRenderBufferInternal(RenderBuffer& pRenderBuffer) = 0;

	virtual bool	AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) = 0;
	virtual void	ReleaseTextureResources(struct Texture* pTexture) = 0;

	virtual bool	CreateShader(class Shader* pShader, struct ShaderConfig_t& pAppConfig) = 0;
	virtual void	DestroyShader(class Shader* pShader) = 0;

	virtual void	AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName) = 0;
	virtual void	ReleaseShaderInstanceResources(class Shader* pShader, unsigned int pInstanceID) = 0;

	virtual void	BindShader(class Shader* pShader) = 0;

	virtual void	BindPerFrameShaderDescriptors(class Shader* pShader) = 0;
	virtual void	BindPerInstanceShaderDescriptors(class Shader* pShader, unsigned int hInstanceID) = 0;

	virtual void	SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) = 0;

	virtual void	AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, uint64_t iVertexBase, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) = 0;
	virtual void	ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, struct GeometryInstance_t& pGeometry) = 0;

	virtual void	UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, struct GeometryConfig_t& pNewGeometryConfig) = 0;
	virtual void	UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, struct GeometryConfig2D_t& pNewGeometryConfig) = 0;
	virtual void	UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) = 0;
};