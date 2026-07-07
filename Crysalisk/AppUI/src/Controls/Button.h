//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A button, which can fire events and have a label attached to it.
//=============================================================================
#pragma once

#include <Defines.h>

#include "AppUI.h"
#include "UI_Types.h"

namespace AppUI
{
	void				CreateButton(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlButton_t* pButtonControl, AppUIRenderContext_t* pRenderContext);
} // namespace AppUI
