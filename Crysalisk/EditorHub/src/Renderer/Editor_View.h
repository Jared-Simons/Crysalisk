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

struct EditorViewExtendedData_t
{
	Utl::CVector<struct GeometryInstance_t*> geo{};
};

// Factory function to create the view.
class IRenderView* RenderViewCreate_EditorView();