//=============================================================================
// Author: Jared Simons
// Date Created: 7/25/2026
// 
// Purpose: A panel with the ability to render images.
//=============================================================================
#pragma once

#include <Defines.h>

#include "AppUI.h"
#include "UI_Types.h"

namespace AppUI
{
	void				CreateImagePanel(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext);

	void				SetImagePanelImage(UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext, TextureHandle_t hImage);

} // namespace AppUI
