//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A button, which can fire events and have a label attached to it.
//=============================================================================
#include "Panel.h"

// Core
#include <Core/Assertions.h>

// Engine
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	static void GenerateControlGeometry(BaseUIControl* pUIBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);
	static void TestButtonClickedCallback();

	void CreateButton(BaseUIControl* pBase, UIControlRenderData_t* pRenderable, ButtonControlData_t* pButtonControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pButtonControl && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_BUTTON;

		// Set some defaults here, the caller can override after creation.
		pBase->x = 0;
		pBase->y = 0;
		pBase->width = 100;
		pBase->height = 100;
		pBase->color = Vector4D{ 0.4f, 0.4f, 0.4f, 0.9f };

		pButtonControl->PFN_UI_Button_Control_Clicked = TestButtonClickedCallback;

		// Create the geometry resources.
		GenerateControlGeometry(pBase, pRenderable, pRenderContext);
	}

	static void GenerateControlGeometry(BaseUIControl* pUIBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pUIBase && pRenderable && pRenderContext);

		// Release any existing geo resources.
		if (pRenderable->geo.vertexCount != kInvalidID_U64)
		{
			pRenderContext->m_pRenderer->ReleaseGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer, pRenderable->geo);
		}

		Vector2D	verts[4];
		u32			indices[6];

		verts[0] = { (f32)pUIBase->x, (f32)pUIBase->y };
		verts[1] = { (f32)pUIBase->x + (f32)pUIBase->width, (f32)pUIBase->y + (f32)pUIBase->height };
		verts[2] = { (f32)pUIBase->x, (f32)pUIBase->y + (f32)pUIBase->height };
		verts[3] = { (f32)pUIBase->x + (f32)pUIBase->width, (f32)pUIBase->y };

		indices[0] = 2;
		indices[1] = 1;
		indices[2] = 0;
		indices[3] = 3;
		indices[4] = 0;
		indices[5] = 1;

		// FIXME: Would be nice to not have to use GeometryInstance_t everywhere.
		GeometryInstance_t outGeometryInstance{};
		pRenderContext->m_pRenderer->AcquireGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer,
			4, sizeof(Vector2D), 0, verts,
			6, indices, &outGeometryInstance);

		pRenderable->vertexCount = outGeometryInstance.vertexCount;
		pRenderable->vertexDataOffset = outGeometryInstance.vertexDataOffset;
		pRenderable->indexCount = outGeometryInstance.indexCount;
		pRenderable->indexDataOffset = outGeometryInstance.indexDataOffset;
		pRenderable->geo = outGeometryInstance;

		// Acquire shader resources.
		ShaderHandle_t hShader = pRenderContext->m_pShaderSystem->AcquireShader("App_UI");
		CAssert(hShader != kInvalidID_U32);
		pRenderable->hShader = hShader;
		pRenderContext->m_pShaderSystem->AcquireShaderInstanceResources(hShader, &pRenderable->iShaderInstance, "App_UI_Button");
		pRenderContext->m_pShaderSystem->SetShaderUniform(hShader,
			pRenderContext->m_pShaderSystem->GetShaderUniformLocation(hShader, "color"), 0, pRenderable->iShaderInstance, &pUIBase->color);
	}

	static void TestButtonClickedCallback()
	{
		LOG_DEBUG("BUTTON PRESSED!");
	}

} // namespace AppUI
