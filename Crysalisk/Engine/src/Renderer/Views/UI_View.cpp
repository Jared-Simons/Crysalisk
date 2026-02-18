//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2026
// 
// Purpose: Interface for the main UI rendering view.
//=============================================================================
#include "UI_View.h"

#include <Core/Engine.h>
#include <Renderer/Renderer.h>

class CUIRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "UI View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	Renderpass						m_pRenderpass{};
	IRenderer*						m_pRenderer{ nullptr };
};

bool CUIRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	CAssert(pEngineSystems != nullptr);

	// Store off subsystem pointers.
	m_pRenderer = pEngineSystems->pRenderer;

	// Setup the renderpass.
	RenderpassConfig_t UIPassConfig{};
	UIPassConfig.pszName = "UI View";

	// This view renders to 2 attachments: color and depth.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

	UIPassConfig.attachmentConfigs.PushBack(colorAttachment);
	UIPassConfig.attachmentConfigs.PushBack(depthAttachment);
	UIPassConfig.clearFlags = RENDERPASS_CLEAR_DEPTH | RENDERPASS_CLEAR_STENCIL;
	UIPassConfig.bPresentsAfter = true;

	if (!m_pRenderer->CreateRenderpass(&UIPassConfig, &m_pRenderpass))
	{
		LOG_ERROR("Failed to create UI Renderpass!");
		return false;
	}

	// Register with the renderer.
	pEngineSystems->pRenderer->RegisterRenderView(this);

	return true;
}

void CUIRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CUIRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		// Draw stuff here.
	}
	m_pRenderer->EndRenderpass();
}

// Factory function to create the view.
IRenderView* RendererViewCreate_UIView()
{
	CUIRenderView* pNewView = new CUIRenderView;
	return pNewView;
}
