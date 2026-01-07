//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2025
// 
// Purpose: The Vulkan Renderer interface.
//=============================================================================
#pragma once

#include "pch.h"
#include "Math/MathLib.h"

//-------------------------------------------------------------------------
// Geometry instance data.
//-------------------------------------------------------------------------
struct GeometryInstanceData_t
{
	size_t	vertexCount{ 0 };
	size_t	vertexDataOffset{ UINT64_MAX };
	size_t	vertexDataSize{ UINT64_MAX };
};

//-------------------------------------------------------------------------
// Geometry render flags.
//-------------------------------------------------------------------------
enum eGeometryRenderFlags
{
	GEOMETRY_RENDER_FLAG_UNDEFINED = 0x0,
	GEOMETRY_RENDER_FLAG_CONTROL_HOVERED_OVER = 0x1,
};

//-------------------------------------------------------------------------
// The renderer interface.
//-------------------------------------------------------------------------
class IVulkanRenderer
{
public:
	virtual bool	Initialize(void* pHWND, uint32_t inWidth, uint32_t inHeight) = 0;
	virtual void	Shutdown() = 0;

	virtual void	BeginFrame() = 0;
	virtual void	EndFrame() = 0;

	virtual void	RenderGeometry(GeometryInstanceData_t& pGeometry, uint8_t iFlags) = 0;

	virtual void	GetRenderSurfaceDimensions(uint32_t& pOutWidth, uint32_t& pOutHeight) = 0;

	virtual GeometryInstanceData_t	AcquireGeometryResources(uint32_t vertCount, Vector2D* pVertices) = 0;
};

// The Vulkan renderer context.
extern IVulkanRenderer* g_pRenderer;

const int kMaxFramesInFlight = 2;
