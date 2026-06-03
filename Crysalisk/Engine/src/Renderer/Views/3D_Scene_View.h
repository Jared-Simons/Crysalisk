//=============================================================================
// Author: Jared Simons
// Date Created: 1/23/2026
// 
// Purpose: Interface for the main scene rendering view.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include <Defines.h>
#include <Renderer/RendererTypes.h>

struct MainSceneView_ExtendedData_t
{
	Utl::CVector<struct GeometryInstance_t*>	pOpaqueGeometries{};
	Utl::CVector<struct GeometryInstance_t*>	pTransparentGeometries{};
};

DLL_EXPORT IRenderView*	RenderViewCreate_3DSceneView();
