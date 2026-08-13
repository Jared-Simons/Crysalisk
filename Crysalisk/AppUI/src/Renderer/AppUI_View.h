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

namespace AppUI
{
	struct AppUIView_ExtendedData_t
	{
		Utl::CVector<struct UIControlRenderData_t*> renderables{};
		Utl::CVector<struct UIImageRenderData_t*>	imageRenderData{};

		RenderBuffer* pVertexBuffer{ nullptr };
		RenderBuffer* pIndexBuffer{ nullptr };

		Matrix4x4	projectionMatrix{ 1.0f };
	};

	IRenderView* RenderViewCreate_AppUIView();
} // namespace AppUI
