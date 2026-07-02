//=============================================================================
// Author: Jared Simons
// Date Created: 6/22/2026
// 
// Purpose: A label, used for rendering text.
//=============================================================================
#pragma once

#include <Defines.h>

#include "AppUI.h"
#include "UI_Types.h"

namespace AppUI
{
	void				CreateLabelControl(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlLabel_t* pLabel, AppUIRenderContext_t* pRenderContext);

	void				SetLabelPosition(UIControlLabel_t* pLabelData, u16 x, u16 y);
	void				SetLabelFont(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, const char* pszFontName, f32 flFontSize);
	void				SetLabelText(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, const char* pszText);

} // namespace AppUI
