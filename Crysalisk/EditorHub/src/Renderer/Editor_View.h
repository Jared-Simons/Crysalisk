//=============================================================================
// Author: Jared Simons
// Date Created: 7/17/2026
// 
// Purpose: Renderview for Editor geometry.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Containers/CVector.h>

struct EditorView_ExtendedData_t
{
	struct GeometryInstance_t*					gizmoGeo{};
	Utl::CVector<struct GeometryInstance_t*>	editorGeo{};
	Utl::CVector<struct GeometryInstance_t*>	mapMeshes{};
};

// Factory function to create the view.
class IRenderView* RenderViewCreate_EditorView();