//=============================================================================
// Author: Jared Simons
// Date Created: 6/22/2026
// 
// Purpose: A frame, which can house/have other controls parented to.
//=============================================================================
#pragma once

#include <Defines.h>

#include "AppUI.h"
#include "UI_Types.h"

namespace AppUI
{
	void				CreateFrameControl(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, UIControlFrame_t* pFrameControl, AppUIRenderContext_t* pRenderContext);
} // namespace AppUI
