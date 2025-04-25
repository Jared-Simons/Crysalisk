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
	virtual bool	RendererBackend_Initialize(struct RendererConfig_t& pConfig) = 0;
	virtual void	RendererBackend_Shutdown() = 0;

	virtual bool	StartFrame(struct RenderPacket_t& pRenderData) = 0;
	virtual void	EndFrame(struct RenderPacket_t& pRenderData) = 0;
	virtual bool	StartRenderpass(eKnownRenderpassType renderpassType) = 0;
	virtual void	EndRenderpass() = 0;

	virtual void	DrawGeometry(struct GeometryInstance_t& pGeometry, struct MaterialReference& pMaterial) = 0;
	virtual void	RenderTestUI() = 0;

	virtual bool	AcquireTextureResources(class Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) = 0;
	virtual void	ReleaseTextureResources(class Texture& pTexture) = 0;

	virtual	bool	AcquireMaterialResources(class Material& pMaterial, struct MaterialInstanceData_t& pMaterialInstanceData) = 0;

	virtual bool	AcquireShaderResources(class Shader& pShader, struct ShaderConfig_t& pShaderConfig) = 0;
	virtual void	ReleaseShaderResources(class Shader& pShader) = 0;

	virtual void	BindShader(class Shader& pShader) = 0;

	virtual bool	AcquireGeometryResources(struct GeometryConfig_t& pGeometryConfig, struct GeometryInstance_t& pOutInstance) = 0;
	virtual bool	AcquireGeometryResources(struct GeometryConfig2D_t& pGeometryConfig, struct GeometryInstance_t& pOutInstance) = 0;
};