//=============================================================================
// Author: Jared Simons
// Date Created: 7/17/2026
// 
// Purpose: Renderview for Editor geometry.
//=============================================================================
#include "Editor_View.h"

// Engine
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Systems/MaterialSystem.h>
#include <Systems/ShaderSystem.h>

class CEditorView : public IRenderView
{
public:
	virtual bool Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void Shutdown(IRenderer* pRenderer);

	virtual void BuildRenderPacket(RenderPacket_t* pRenderData) override;
	virtual void Render(RenderPacket_t* pRenderData) override;

	virtual const char* GetViewName() const override { return "Editor View"; }
	virtual const Renderpass* GetRenderpass() const override { return &m_Renderpass; }

private:
	Renderpass	m_Renderpass{};
};

bool CEditorView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	IRenderer* pRenderer = pEngineSystems->pRenderer;

	RenderpassConfig_t editorPassConfig{};
	editorPassConfig.pszName = "Editor";

	// This pass has 2 attachments: color and depth.
	// NOTE: The depth attachment is temporary, we do not actually use the contents of the depth buffer during this pass.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

	editorPassConfig.attachmentConfigs.PushBack(colorAttachment);
	editorPassConfig.attachmentConfigs.PushBack(depthAttachment);
	editorPassConfig.clearFlags = RENDERPASS_CLEAR_NONE;
	editorPassConfig.bPresentsAfter = false;
	pRenderer->CreateRenderpass(&editorPassConfig, &m_Renderpass);

	// Register with the renderer.
	pRenderer->RegisterRenderView(this);

	return true;
}

void CEditorView::Shutdown(IRenderer* pRenderer)
{
	if (m_Renderpass.pInternalData)
	{
		pRenderer->DestroyRenderpass(&m_Renderpass);
	}
}

void CEditorView::BuildRenderPacket(RenderPacket_t* pRenderData)
{
}

void CEditorView::Render(RenderPacket_t* pRenderData)
{
	IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
	IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;

	if (pRenderer->BeginRenderpass(&m_Renderpass))
	{
		if (pRenderData->pEditorViewExtendedData)
		{
			EditorView_ExtendedData_t* pViewData = (EditorView_ExtendedData_t*)pRenderData->pEditorViewExtendedData;

			// HACK: Editor test!
			// TODO: Make this a helper method?
			for (u32 iGeo = 0; iGeo < pViewData->editorGeo.Count(); iGeo++)
			{
				// Don't draw if the data is null.
				// TODO: This shouldn't get populated if null, handle this when querying for geometry data.
				if (pViewData->editorGeo[iGeo] == nullptr)
				{
					continue;
				}
				
				// HACK: Editor hack!
				static RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
				static RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

				GeometryInstance_t* pCurGeo = pViewData->editorGeo[iGeo];

				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "projection"), 0, 0, &pRenderData->matProjection);
				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "view"), 0, 0, &pRenderData->pCamera->GetViewMatrix());
				pShaderSystem->BindShader(pCurGeo->hShader);
				if (pCurGeo->hShaderInstance != kInvalidID_U32)
				{
					pShaderSystem->BindPerInstanceUniforms(pCurGeo->hShaderInstance);
				}
				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "model"),	// FIXME: Strings are slow.
					0, pCurGeo->hShaderInstance, &pCurGeo->matLocalToWorldTransform);
				
				pRenderer->DrawGeometry(*pCurGeo, pVertexBuffer, pIndexBuffer);
			}

			// Render the gizmo.
			if (pViewData->gizmoGeo)
			{
				// Disable depth testing for the gizmo.
				pRenderer->SetDepthTestEnable(false);

				static RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
				static RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

				GeometryInstance_t* pCurGeo = pViewData->gizmoGeo;

				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "projection"), 0, 0, &pRenderData->matProjection);
				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "view"), 0, 0, &pRenderData->pCamera->GetViewMatrix());
				pShaderSystem->BindShader(pCurGeo->hShader);
				if (pCurGeo->hShaderInstance != kInvalidID_U32)
				{
					pShaderSystem->BindPerInstanceUniforms(pCurGeo->hShaderInstance);
				}
				pShaderSystem->SetShaderUniform(pCurGeo->hShader, pShaderSystem->GetShaderUniformLocation(pCurGeo->hShader, "model"),	// FIXME: Strings are slow.
					0, pCurGeo->hShaderInstance, &pCurGeo->matLocalToWorldTransform);

				pRenderer->DrawGeometry(*pCurGeo, pVertexBuffer, pIndexBuffer);

				// Restore depth state.
				pRenderer->SetDepthTestEnable(true);
			}

			// Render current map doc meshes.
			IMaterialSystem* pMaterialSystem = g_pEngine->GetEngineInterfaces()->pMaterialSystem;
			for (u32 iGeo = 0; iGeo < pViewData->mapMeshes.Count(); iGeo++)
			{
				// HACK: Editor hack!
				static RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("World_Vertex_Buffer");
				static RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("World_Index_Buffer");

				GeometryInstance_t* pCurGeo = pViewData->mapMeshes[iGeo];

				if (pCurGeo->hShaderInstance != kInvalidID_U32)
				{
					pMaterialSystem->ApplyPerDrawRenderData(pCurGeo->matLocalToWorldTransform);

					pMaterialSystem->ApplyMaterialInstance(pCurGeo->hShaderInstance);

					pRenderer->DrawGeometry(*pCurGeo, pVertexBuffer, pIndexBuffer);

					pRenderData->pMetrics->NumDrawCalls++;
				}
			}

			pViewData->editorGeo.Purge();
			pViewData->mapMeshes.Purge();
		}

		pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_EditorView()
{
	return new CEditorView;
}