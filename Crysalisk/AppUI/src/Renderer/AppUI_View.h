//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Render view for the AppUI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Containers/CVector.h>

// Engine
#include <Renderer/RendererTypes.h>

// Forward declarations
struct GeometryInstance_t;

namespace AppUI
{
	struct AppUIView_ExtendedData_t
	{
		Utl::CVector<struct GeometryInstance_t> renderables{};
	};

	IRenderView* RenderViewCreate_AppUIView();
} // namespace AppUI
