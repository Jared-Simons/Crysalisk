//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A button, which can fire events and have a label attached to it.
//=============================================================================
#include "Panel.h"

// Core
#include <Core/Assertions.h>

// Engine
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	static void TestButtonClickedCallback();

	void CreateButton(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, UIControlButton_t* pButtonControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pButtonControl && pRenderContext);

		// FIXME: Make this work
		//pBase->m_eControlType = CONTROL_TYPE_BUTTON;

		// Set some defaults here, the caller can override after creation.
		pBase->x = 0;
		pBase->y = 0;
		pBase->width = 100;
		pBase->height = 100;
		pBase->color = Vector4D{ 0.4f, 0.4f, 0.4f, 0.9f };

		pButtonControl->pPanel = pBase;
		pButtonControl->PFN_UI_Button_Control_Clicked = TestButtonClickedCallback;
	}

	static void TestButtonClickedCallback()
	{
		LOG_DEBUG("BUTTON PRESSED!");
	}

} // namespace AppUI
