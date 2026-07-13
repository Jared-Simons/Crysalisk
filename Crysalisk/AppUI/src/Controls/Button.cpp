//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A button, which can fire events and have a label attached to it.
//=============================================================================
#include "Panel.h"

#include "Label.h"

// Core
#include <Core/Assertions.h>

namespace AppUI
{
	void CreateButton(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlButton_t* pButtonControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pRenderable && pButtonControl && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_BUTTON;

		// Set some defaults here, the caller can override after creation.
		pButtonControl->panel.x = 0;
		pButtonControl->panel.y = 0;
		pButtonControl->panel.width = 100;
		pButtonControl->panel.height = 100;
		pButtonControl->panel.color = Vector4D{ 0.4f, 0.4f, 0.4f, 0.9f };

		pButtonControl->onPressedCallback = nullptr;
	}

} // namespace AppUI
