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

	if (pRenderer->BeginRenderpass(&m_Renderpass))
	{
		if (pRenderData->pEditorViewExtendedData)
		{
			// HACK: Editor hack!
			static RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
			static RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

			GeometryInstance_t* pTypedGizmo = (GeometryInstance_t*)pRenderData->pEditorViewExtendedData;
	
			IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;
			pShaderSystem->BindShader(pTypedGizmo->hShader);
			pShaderSystem->BindPerInstanceUniforms(pTypedGizmo->hShaderInstance);
			pShaderSystem->SetShaderUniform(pTypedGizmo->hShader, pShaderSystem->GetShaderUniformLocation(pTypedGizmo->hShader, "model"),	// FIXME: Strings are slow.
											0, pTypedGizmo->hShaderInstance, &pTypedGizmo->matLocalToWorldTransform);
			
			pRenderer->DrawGeometry(*pTypedGizmo, pVertexBuffer, pIndexBuffer);
		}

		pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_EditorView()
{
	return new CEditorView;
}