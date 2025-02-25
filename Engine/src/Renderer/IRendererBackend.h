//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The interface to the renderer backend
// Not to be accessed directly! Rather through the renderer frontend
//=============================================================================
#pragma once

#include "Defines.h"

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

	virtual void	DrawGeometry(struct GeometryInstance_t& pGeometry) = 0;

	virtual bool	AcquireTextureResources(class Texture& pTexture, size_t nPixelDataSize, byte* pPixelData) = 0;

	virtual bool	AcquireGeometryResources(struct GeometryConfig_t& pGeometryConfig, struct GeometryInstance_t& pOutInstance) = 0;
};