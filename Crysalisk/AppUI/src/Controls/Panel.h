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
	void				CreatePanel(UIBaseControl_t* pBaseControl, UIControlRenderData_t* pRenderable, UIControlPanel_t* pPanelControl, AppUIRenderContext_t* pRenderContext);
	void				DestroyPanel(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);

	void				SetPanelControlPosition(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 x, u16 y, bool bAcquireShaderResources);
	void				SetPanelControlDimensions(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 width, u16 height, bool bAcquireShaderResources);

} // namespace AppUI
