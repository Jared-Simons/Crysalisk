//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A base panel, simply used for parenting other controls to.
//=============================================================================
#include "Panel.h"

// Core
#include <Core/Assertions.h>

// Engine
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	static void GenerateControlGeometry(UIControlPanel_t* pUIBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);

	void CreatePanel(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_PANEL;
		
		// Set some defaults here, the caller can override after creation.
		pBase->x = 0;
		pBase->y = 0;
		pBase->width = 100;
		pBase->height = 100;
		pBase->color = Vector4D{ 0.4f, 0.4f, 0.4f, 0.9f };
	}

	void DestroyPanel(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pRenderContext);

		// Release shader resources.
		pRenderContext->m_pShaderSystem->ReleaseShaderInstanceResources(pRenderable->hShader, pRenderable->iShaderInstance);

		// Release geometry resources.
		pRenderContext->m_pRenderer->ReleaseGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer, pRenderable->geo);

		// Zero out structures.
		// Note that this will set controlType back to CONTROL_TYPE_UNDEFINED, allowing for this to be reused.
		Platform::MemorySet(pRenderable, 0, sizeof(UIControlRenderData_t));
		Platform::MemorySet(pBase, 0, sizeof(UIControlPanel_t));
		pRenderable->vertexCount = kInvalidID_U64;
	}

	void SetPanelControlPosition(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 x, u16 y)
	{
		pBase->x = x;
		pBase->y = y;
		GenerateControlGeometry(pBase, pRenderable, pRenderContext);
	}

	void SetPanelControlDimensions(UIControlPanel_t* pBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext, u16 width, u16 height)
	{
		pBase->width = width;
		pBase->height = height;
		GenerateControlGeometry(pBase, pRenderable, pRenderContext);
	}

	static void GenerateControlGeometry(UIControlPanel_t* pUIBase, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
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
		pRenderContext->m_pShaderSystem->AcquireShaderInstanceResources(hShader, &pRenderable->iShaderInstance, "App_UI_Panel");
		pRenderContext->m_pShaderSystem->SetShaderUniform(hShader,
			pRenderContext->m_pShaderSystem->GetShaderUniformLocation(hShader, "color"), 0, pRenderable->iShaderInstance, &pUIBase->color);
	}

} // namespace AppUI
