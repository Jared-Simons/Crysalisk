//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A base panel, simply used for parenting other controls to.
//=============================================================================
#include "Panel.h"

// Core
#include <Core/Assertions.h>

namespace AppUI
{
	struct PanelControlData
	{
		u16 x;
		u16 y;
		u16 width;
		u16 height;
	};

	static void GenerateControlGeometry(PanelControlData* pPanelData, GeometryInstance_t* pOutGeometryInstance, IRenderer* pRenderer, IShaderSystem* pShaderSystem, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer);

	BaseUIControl* CreatePanel(u16 x, u16 y, u16 width, u16 height, IRenderer* pRenderer, IShaderSystem* pShaderSystem, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer)
	{
		// FIXME: Get this from a factory or something so they're all grouped.
		BaseUIControl* pNewBase = new BaseUIControl;
		pNewBase->m_eControlType = CONTROL_TYPE_PANEL;

		pNewBase->m_pControlData = new PanelControlData;
		PanelControlData* pInternalData = (PanelControlData*)pNewBase->m_pControlData;
		pInternalData->x = x;
		pInternalData->y = y;
		pInternalData->width = width;
		pInternalData->height = height;

		GenerateControlGeometry(pInternalData, &pNewBase->m_Geometry, pRenderer, pShaderSystem, pVertexBuffer, pIndexBuffer);

		return pNewBase;
	}

	void DestroyPanel(BaseUIControl* pPanel)
	{
		CAssert(pPanel && pPanel->m_pControlData);

		PanelControlData* pInternalData = (PanelControlData*)pPanel->m_pControlData;
		delete pInternalData;
		pInternalData = nullptr;

		delete pPanel;
	}

	static void GenerateControlGeometry(PanelControlData* pPanelData, GeometryInstance_t* pOutGeometryInstance, IRenderer* pRenderer, IShaderSystem* pShaderSystem, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer)
	{
		CAssert(pPanelData && pOutGeometryInstance && pRenderer && pShaderSystem);

		Vector2D	verts[4];
		u32			indices[6];

		verts[0] = { (f32)pPanelData->x, (f32)pPanelData->y };
		verts[1] = { (f32)pPanelData->x + (f32)pPanelData->width, (f32)pPanelData->y + (f32)pPanelData->height };
		verts[2] = { (f32)pPanelData->x, (f32)pPanelData->y + (f32)pPanelData->height };
		verts[3] = { (f32)pPanelData->x + (f32)pPanelData->width, (f32)pPanelData->y };

		indices[0] = 2;
		indices[1] = 1;
		indices[2] = 0;
		indices[3] = 3;
		indices[4] = 0;
		indices[5] = 1;

		pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, 4, sizeof(Vector2D), 0, verts, 6, indices, pOutGeometryInstance);
	
		ShaderHandle_t shader = pShaderSystem->AcquireShader("App_UI");	// FIXME: Don't get the shader every time.
		pShaderSystem->AcquireShaderInstanceResources(shader, &pOutGeometryInstance->hShaderInstance, "App_UI_Panel");
		Vector4D color{ 0.4f, 0.4f, 0.4f, 0.9f };
		pShaderSystem->SetShaderUniform(shader,
			pShaderSystem->GetShaderUniformLocation(shader, "color"), 0, pOutGeometryInstance->hShaderInstance, &color);
	}

} // namespace AppUI
