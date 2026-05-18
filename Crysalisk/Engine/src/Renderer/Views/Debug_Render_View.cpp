//=============================================================================
// Author: Jared Simons
// Date Created: 5/17/2026
// 
// Purpose: Implementation of the debug rendering view.
//=============================================================================
#include "Debug_Render_View.h"

#include <Core/Engine.h>
#include <Renderer/Renderer.h>

class CDebugRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "Debug View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	Renderpass						m_pRenderpass{};

	IRenderer*						m_pRenderer{ nullptr };
};

bool CDebugRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	m_pRenderer = pEngineSystems->pRenderer;

	// Setup the debug view renderpass.
	RenderpassConfig_t DebugPassConfig{};
	DebugPassConfig.pszName = "Debug Renderer";

	// This pass has 2 attachments: color and depth.
	// NOTE: The depth attachment is temporary, we do not actually use the contents of the depth buffer during this pass.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

	DebugPassConfig.attachmentConfigs.PushBack(colorAttachment);
	DebugPassConfig.attachmentConfigs.PushBack(depthAttachment);
	DebugPassConfig.clearFlags = RENDERPASS_CLEAR_NONE;
	DebugPassConfig.bPresentsAfter = false;

	if (!m_pRenderer->CreateRenderpass(&DebugPassConfig, &m_pRenderpass))
	{
		LOG_ERROR("Failed to create UI Renderpass!");
		return false;
	}

	// Register with the renderer.
	m_pRenderer->RegisterRenderView(this);

	return true;
}

void CDebugRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CDebugRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		m_pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_DebugView()
{
	return new CDebugRenderView;
}
