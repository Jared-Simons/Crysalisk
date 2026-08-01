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

struct MeshDrawCommand_t
{
	u32 meshInstance{ kInvalidID_U32 };
	u32 submeshInstance{ kInvalidID_U32 };
};

struct MainSceneView_ExtendedData_t
{
	Utl::CVector<MeshDrawCommand_t>	pOpaqueGeometries{};
	Utl::CVector<MeshDrawCommand_t>	pTransparentGeometries{};
};

DLL_EXPORT IRenderView*	RenderViewCreate_3DSceneView();
