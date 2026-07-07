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
		pLabel->text.m_hFont = pFontSystem->GetDefaultFont();
		pLabel->text.m_vecPos = { 100.0f, 500.0f };	// LEFTOFF: Figure out the dimensions to use here. (1080x720?)

		pLabel->text.m_Geometry.hShader = pShaderSystem->AcquireShader("Core.UI");
		pLabel->text.m_Geometry.hShaderInstance = pLabel->text.m_hFont; // FIXME: Get this from somewhere better?
		pLabel->hFont = pLabel->text.m_hFont;
		pRenderable->hShader = pLabel->text.m_Geometry.hShader;
		pRenderable->iShaderInstance = pLabel->text.m_Geometry.hShaderInstance;
	}

	void SetLabelPosition(UIControlLabel_t* pLabelData, u16 x, u16 y)
	{
		CAssert(pLabelData);

		IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;

		pLabelData->text.m_vecPos = { (f32)x, (f32)y };
		pLabelData->text.SetText(pRenderer, pFontSystem, pLabelData->text.m_pszText);	// NOTE: We have to call this again because this is the only way to regenerate text geo currently.
	}

	void SetLabelFont(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, const char* pszFontName, f32 flFontSize)
	{
		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;
		pLabelData->hFont = pFontSystem->AcquireFontResources(pszFontName, flFontSize);
		pLabelData->text.m_Geometry.hShaderInstance = pLabelData->hFont; // FIXME: Get this from somewhere better?
		pRenderable->iShaderInstance = pLabelData->text.m_Geometry.hShaderInstance;
	}

	void SetLabelText(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, const char* pszText)
	{
		CAssert(pLabelData && pRenderable && pszText);

		if (pLabelData->pszText)
		{
			Utl::StringFree(pLabelData->pszText);
		}

		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;

		// Quick check: If the string is empty, use a space instead.
		if (Utl::StringLength(pszText) < 1)
		{
			pszText = " ";
		}

		pLabelData->pszText = Utl::StringDuplicate(pszText);
		GeometryConfig2D_t textGeoConfig = pFontSystem->AcquireConfigForText(pLabelData->hFont, pLabelData->text.m_vecPos, pszText);	// FIXME: Store pos somewhere else.

		if (textGeoConfig.Vertices.Count() > 0)
		{
			pRenderContext->m_pRenderer->AcquireGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer,
				textGeoConfig.Vertices.Count(), textGeoConfig.Vertices.Stride(), 0, textGeoConfig.Vertices.Data(),
				textGeoConfig.Indices.Count(), textGeoConfig.Indices.Data(), &pRenderable->geo);
		}
	}

	void RegenerateLabelGeometry(UIControlLabel_t* pLabelData, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pLabelData && pRenderable && pRenderContext);

		// Release any existing geo resources.
		if (pRenderable->geo.vertexCount != kInvalidID_U64)
		{
			pRenderContext->m_pRenderer->ReleaseGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer, pRenderable->geo);
		}

		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;

		GeometryConfig2D_t textGeoConfig = pFontSystem->AcquireConfigForText(pLabelData->hFont, pLabelData->text.m_vecPos, pLabelData->pszText);	// FIXME: Store pos somewhere else.
		pRenderContext->m_pRenderer->AcquireGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer,
			textGeoConfig.Vertices.Count(), textGeoConfig.Vertices.Stride(), 0, textGeoConfig.Vertices.Data(),
			textGeoConfig.Indices.Count(), textGeoConfig.Indices.Data(), &pRenderable->geo);
	}

} // namespace AppUI
