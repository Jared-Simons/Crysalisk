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

// Core
#include <Resources/Loaders/FontLoader.h>

namespace AppUI
{
	void				CreateImagePanel(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext);

	void				SetImagePanelImage(UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext, TextureHandle_t hImage, FontLoader::GlyphLayoutData_t* pGlyphData);
	void				RegenerateImagePanelGeometry(UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext);

} // namespace AppUI
