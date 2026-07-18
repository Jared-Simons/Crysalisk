//=============================================================================
// Author: Jared Simons
// Date Created: 7/17/2026
// 
// Purpose: Renderview for Level Editor geometry.
//=============================================================================
#include "Level_Editor_View.h"

// Engine
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>

class CLevelEditorView : public IRenderView
{
public:
	virtual bool Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void Shutdown(IRenderer* pRenderer);

	virtual void BuildRenderPacket(RenderPacket_t* pRenderData) override;
	virtual void Render(RenderPacket_t* pRenderData) override;

	virtual const char* GetViewName() const override { return "Level Editor View"; }
	virtual const Renderpass* GetRenderpass() const override { return &m_Renderpass; }

private:
	Renderpass	m_Renderpass{};
};

bool CLevelEditorView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	IRenderer* pRenderer = pEngineSystems->pRenderer;

	RenderpassConfig_t editorPassConfig{};
	editorPassConfig.pszName = "Level_Editor";

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

void CLevelEditorView::Shutdown(IRenderer* pRenderer)
{
	if (m_Renderpass.pInternalData)
	{
		pRenderer->DestroyRenderpass(&m_Renderpass);
	}
}

void CLevelEditorView::BuildRenderPacket(RenderPacket_t* pRenderData)
{
}

void CLevelEditorView::Render(RenderPacket_t* pRenderData)
{
	IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;

	if (pRenderer->BeginRenderpass(&m_Renderpass))
	{
		pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_LevelEditorView()
{
	return new CLevelEditorView;
}