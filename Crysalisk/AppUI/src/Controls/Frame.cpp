//=============================================================================
// Author: Jared Simons
// Date Created: 6/22/2026
// 
// Purpose: A frame, which can house/have other controls parented to.
//=============================================================================
#include "Frame.h"

// Core
#include <Core/Assertions.h>

// Engine
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	void CreateFrameControl(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, UIControlFrame_t* pFrameControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pFrameControl && pRenderContext);

		// FIXME: Make this work
		//pBase->m_eControlType = CONTROL_TYPE_FRAME;

		// Set some defaults here, the caller can override after creation.
		pBase->x = 0;
		pBase->y = 0;
		pBase->width = 100;
		pBase->height = 100;
		pBase->color = Vector4D{ 0.4f, 0.4f, 0.4f, 0.9f };
	}

} // namespace AppUI
