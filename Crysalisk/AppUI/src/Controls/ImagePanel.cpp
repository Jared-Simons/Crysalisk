//=============================================================================
// Author: Jared Simons
// Date Created: 7/25/2026
// 
// Purpose: A panel with the ability to render images.
//=============================================================================
#include "ImagePanel.h"

// Core
#include <Core/Assertions.h>

// Engine
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>
#include <Systems/TextureSystem.h>

namespace AppUI
{
	void CreateImagePanel(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pImagePanelControl && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_IMAGE_PANEL;

		// LEFTOFF: Image panel will need texture coords.

		// Set some defaults here, the caller can override after creation.
		pImagePanelControl->hImage = g_pEngine->GetEngineInterfaces()->pTextureSystem->GetDefaultDiffuseTexture();
		pImagePanelControl->panel.y = 0;
		pImagePanelControl->panel.width = 100;
		pImagePanelControl->panel.height = 100;
	}

	void SetImagePanelImage(UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext, TextureHandle_t hImage)
	{
		CAssert(pRenderable && pImagePanelControl && pRenderContext);

		// Set some shader instance data.
		IShaderSystem* pShaderSystem = pRenderContext->m_pShaderSystem;
		i32 iTrue = 1;
		pShaderSystem->SetShaderUniform(pRenderable->hShader, pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "instance_uses_textures"), 0, pRenderable->iShaderInstance, &iTrue);
		pShaderSystem->SetShaderUniform(pRenderable->hShader, pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "tex"), 0, pRenderable->iShaderInstance, &pImagePanelControl->hImage);
	}

} // namespace AppUI
