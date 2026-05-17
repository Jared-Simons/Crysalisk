//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2026
// 
// Purpose: Interface for the main UI rendering view.
//=============================================================================
#include "UI_View.h"

#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include "Systems/FontSystem.h"
#include "Systems/ShaderSystem.h"

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
	IShaderSystem*					m_pShaderSystem{ nullptr };
	IFontSystem*					m_pFontSystem{ nullptr };

	ShaderHandle_t					m_hUIShader{ kInvalidID_U32 };
	// TEST TEXT
	TextObject						m_TestText{};
};

bool CUIRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	CAssert(pEngineSystems != nullptr);

	// Store off subsystem pointers.
	m_pRenderer = pEngineSystems->pRenderer;
	m_pShaderSystem = pEngineSystems->pShaderSystem;
	m_pFontSystem = pEngineSystems->pFontSystem;

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

	m_hUIShader = m_pShaderSystem->AcquireShader("Core.UI");

	return true;
}

void CUIRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CUIRenderView::Render(RenderPacket_t* pRenderData)
{
	m_TestText.m_hFont = 0;
	m_TestText.m_flTextSize = 1.0f;
	m_TestText.m_vecPos = { -0.95f, -0.6f };
	m_TestText.SetText(m_pRenderer, m_pFontSystem, "TESTING");
	
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		// Draw stuff here.
		Matrix4x4 matUIProj(1.0f);
		m_pShaderSystem->SetShaderUniform(m_hUIShader, 0, 0, 1, &matUIProj);
		m_pShaderSystem->BindShader(m_hUIShader);
		m_pShaderSystem->BindPerInstanceUniforms(1);
		m_pRenderer->DrawGeometry(m_TestText.m_Geometry, &m_pRenderer->GetUIVertexBuffer(), &m_pRenderer->GetUIIndexBuffer());
	}
	m_pRenderer->EndRenderpass();
}

// Factory function to create the view.
IRenderView* RendererViewCreate_UIView()
{
	CUIRenderView* pNewView = new CUIRenderView;
	return pNewView;
}
