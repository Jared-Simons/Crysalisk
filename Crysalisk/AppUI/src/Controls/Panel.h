//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A base panel, simply used for parenting other controls to.
//=============================================================================
#pragma once

#include <Defines.h>

#include "AppUI.h"
#include "UI_Types.h"

namespace AppUI
{
	void				CreatePanel(BaseUIControl* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);
	void				DestroyPanel(BaseUIControl* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);

	void				SetPanelControlPosition(BaseUIControl* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 x, u16 y);
	void				SetPanelControlDimensions(BaseUIControl* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 width, u16 height);

} // namespace AppUI
