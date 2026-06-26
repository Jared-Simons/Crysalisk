//=============================================================================
// Author: Jared Simons
// Date Created: 6/22/2026
// 
// Purpose: A label, used for rendering text.
//=============================================================================
#include "Label.h"

// Core
#include <Core/Assertions.h>
#include <Strings/String_Utils.h>

// Engine
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Systems/FontSystem.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	void CreateLabelControl(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlLabel_t* pLabel, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pLabel && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_LABEL;

		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;
		IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;
		pLabel->text.m_flTextSize = 0.99f;
		pLabel->text.m_hFont = pFontSystem->AcquireFontResources("Assets/Fonts/UbuntuMono-R.ttf", 30.0f);
		pLabel->text.m_vecPos = { 100.0f, 500.0f };	// LEFTOFF: Figure out the dimensions to use here. (1080x720?)
		pLabel->text.m_Geometry.hShader = pShaderSystem->AcquireShader("Core.UI");
		pLabel->text.m_Geometry.hShaderInstance = 1; // FIXME: Don't hardcode!
	}

	void SetLabelPosition(UIControlLabel_t* pLabelData, u16 x, u16 y)
	{
		CAssert(pLabelData);

		IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;

		pLabelData->text.m_vecPos = { (f32)x, (f32)y };
		pLabelData->text.SetText(pRenderer, pFontSystem, pLabelData->text.m_pszText);	// NOTE: We have to call this again because this is the only way to regenerate text geo currently.
	}

	void SetLabelText(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, const char* pszText)
	{
		CAssert(pLabelData && pRenderable && pszText);

		IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;

		pLabelData->text.SetText(pRenderer, pFontSystem, pszText);
	}

} // namespace AppUI
