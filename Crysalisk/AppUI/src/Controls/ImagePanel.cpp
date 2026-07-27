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
	static void GenerateControlGeometry(UIControlPanel_t* panel, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext);

	void CreateImagePanel(UIBaseControl_t* pBase, UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(pBase && pRenderable && pImagePanelControl && pRenderContext);

		pBase->m_eControlType = CONTROL_TYPE_IMAGE_PANEL;

		// LEFTOFF: Image panel will need texture coords.

		// Set some defaults here, the caller can override after creation.
		pImagePanelControl->hImage = g_pEngine->GetEngineInterfaces()->pTextureSystem->GetDefaultDiffuseTexture();
		pImagePanelControl->panel.x = 0;
		pImagePanelControl->panel.y = 0;
		pImagePanelControl->panel.width = 256;
		pImagePanelControl->panel.height = 256;

		GenerateControlGeometry(&pImagePanelControl->panel, pRenderable, pRenderContext);
	}

	void SetImagePanelImage(UIControlRenderData_t* pRenderable, UIControlImagePanel_t* pImagePanelControl, AppUIRenderContext_t* pRenderContext, TextureHandle_t hImage)
	{
		CAssert(pRenderable && pImagePanelControl && pRenderContext);

		IShaderSystem* pShaderSystem = pRenderContext->m_pShaderSystem;
		if (pRenderable->iShaderInstance == kInvalidID_U32)
		{
			// Need to acquire shader resources.
			pRenderable->hShader = pShaderSystem->AcquireShader("AppUI_ImagePanel");
			pShaderSystem->AcquireShaderInstanceResources(pRenderable->hShader, &pRenderable->iShaderInstance, "AppUI.ImagePanel");
		}

		pImagePanelControl->hImage = g_pEngine->GetEngineInterfaces()->pTextureSystem->GetDefaultDiffuseTexture();
		pShaderSystem->SetShaderUniform(pRenderable->hShader, pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "image"), 0, pRenderable->iShaderInstance, &pImagePanelControl->hImage);
		pImagePanelControl->hImage = g_pEngine->GetEngineInterfaces()->pTextureSystem->AcquireTexture("Assets/Textures/testpanel.png", true);
	}

	static void GenerateControlGeometry(UIControlPanel_t* panel, UIControlRenderData_t* pRenderable, AppUIRenderContext_t* pRenderContext)
	{
		CAssert(panel && pRenderable && pRenderContext);

		// Release any existing geo resources.
		if (pRenderable->geo.vertexCount != kInvalidID_U64)
		{
			pRenderContext->m_pRenderer->ReleaseGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer, pRenderable->geo);
		}

		Vector2D	pos[4];
		Vector2D	texCoord[4];
		u32			indices[6];

		pos[0] = { (f32)panel->x, (f32)panel->y };
		pos[1] = { (f32)panel->x + (f32)panel->width, (f32)panel->y + (f32)panel->height };
		pos[2] = { (f32)panel->x, (f32)panel->y + (f32)panel->height };
		pos[3] = { (f32)panel->x + (f32)panel->width, (f32)panel->y };

		texCoord[0] = { 0.0f, 0.0f };
		texCoord[1] = { 1.0f, 1.0f };
		texCoord[2] = { 0.0f, 1.0f };
		texCoord[3] = { 1.0f, 0.0f };

		indices[0] = 2;
		indices[1] = 1;
		indices[2] = 0;
		indices[3] = 3;
		indices[4] = 0;
		indices[5] = 1;

		u64 vertStride = sizeof(Vector2D) + sizeof(Vector2D);
		Utl::CVector<f32>	vertData(16);	// This vertex stream consists of positions and texcoords.

		for (u8 iVert = 0; iVert < 4; iVert++)
		{
			vertData.PushBack(pos[iVert].x);
			vertData.PushBack(pos[iVert].y);
			vertData.PushBack(texCoord[iVert].x);
			vertData.PushBack(texCoord[iVert].y);
		}
		
		// FIXME: Would be nice to not have to use GeometryInstance_t everywhere.
		GeometryInstance_t outGeometryInstance{};
		pRenderContext->m_pRenderer->AcquireGeometryResources(*pRenderContext->m_pVertexBuffer, *pRenderContext->m_pIndexBuffer,
			4, vertStride, 0, vertData.Data(),
			6, indices, &outGeometryInstance);

		pRenderable->vertexCount = outGeometryInstance.vertexCount;
		pRenderable->vertexDataOffset = outGeometryInstance.vertexDataOffset;
		pRenderable->indexCount = outGeometryInstance.indexCount;
		pRenderable->indexDataOffset = outGeometryInstance.indexDataOffset;
		pRenderable->geo = outGeometryInstance;
	}

} // namespace AppUI
