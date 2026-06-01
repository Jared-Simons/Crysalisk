//=============================================================================
// Author: Jared Simons
// Date Created: 5/17/2026
// 
// Purpose: Interface for the debug rendering view.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Renderer/RendererTypes.h>

struct DebugRenderView_ExtendedData_t
{
	Utl::CVector<GeometryInstance_t*> pGeometries{};

	RenderBuffer*	pVertexBuffer{ nullptr };
	RenderBuffer*	pIndexBuffer{ nullptr };
};

DLL_EXPORT IRenderView* RenderViewCreate_DebugView();
