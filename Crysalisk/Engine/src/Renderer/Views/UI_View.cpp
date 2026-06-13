//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2026
// 
// Purpose: Interface for the main UI rendering view.
//=============================================================================
#include "UI_View.h"

#include <Core/Engine.h>
#include <Core/Event.h>
#include <Renderer/Renderer.h>
#include "Systems/FontSystem.h"
#include "Systems/ShaderSystem.h"

static bool Event_KeyReleased(void* pEventListener, EventData_t data);

class CUIRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) override;
	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "UI View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	bool							m_bUIOverlayEnabled{ true };

	RenderBuffer*					m_pUIVertexBuffer{ nullptr };
	RenderBuffer*					m_pUIIndexBuffer{ nullptr };

	Renderpass						m_pRenderpass{};
	IRenderer*						m_pRenderer{ nullptr };
	IShaderSystem*					m_pShaderSystem{ nullptr };
	IFontSystem*					m_pFontSystem{ nullptr };

	ShaderHandle_t					m_hUIShader{ kInvalidID_U32 };
	// TEST TEXT
	TextObject						m_TestText{};

	// This needs access to our members.
	friend static bool Event_KeyReleased(void* pEventListener, EventData_t data);
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

	// Get our renderbuffers.
	m_pUIVertexBuffer = m_pRenderer->GetRenderBufferByName("UI_Vertex_Buffer");
	if (m_pUIVertexBuffer == nullptr)
	{
		LOG_ERROR("UI View failed to initialize!");
		return false;
	}

	m_pUIIndexBuffer = m_pRenderer->GetRenderBufferByName("UI_Index_Buffer");
	if (m_pUIIndexBuffer == nullptr)
	{
		LOG_ERROR("UI View failed to initialize!");
		return false;
	}

	m_hUIShader = m_pShaderSystem->AcquireShader("Core.UI");
	m_TestText.m_hFont = 0;
	m_TestText.m_flTextSize = 1.0f;
	m_TestText.m_vecPos = { 25.0f, 575.0f };

	pEngineSystems->pEngineEventHandler->EventRegister(eEventCategory::INPUT_KEY_RELEASED, this, Event_KeyReleased);

	return true;
}

void CUIRenderView::Shutdown(IRenderer* pRenderer)
{
	g_pEngine->GetEngineInterfaces()->pEngineEventHandler->EventUnregister(eEventCategory::INPUT_KEY_RELEASED, this, Event_KeyReleased);
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CUIRenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
{

}

void CUIRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_bUIOverlayEnabled)
	{
		RenderMetrics_t& pMetrics = *pRenderData->pMetrics;
		char szMetricsString[512]{};
		Utl::StringFormat_V("Frametime: %.5f s (FPS: %d)\nDraw calls: %d (Geometry culled: %d)\nCurrent memory pool usage: %.5f\nPosition: %.3f, %.3f, %.3f\nDebug vis mode: %s",
			szMetricsString,
			pMetrics.FrameTime, pMetrics.PreviousFrameFPS,
			pMetrics.NumDrawCalls, pRenderData->iCulledGeometries,
			g_pMemoryManager->GetPercentAllocated(),
			pRenderData->pCamera->vecPosition.x,
			pRenderData->pCamera->vecPosition.y,
			pRenderData->pCamera->vecPosition.z,
			m_pRenderer->GetRendererDebugVisModeName());
		m_TestText.SetText(m_pRenderer, m_pFontSystem, szMetricsString);
	}

	Matrix4x4 matProjection = Matrix4x4::Orthographic(0, 1080.0f, 0.0f, 720.0f, 0.1f, 1000.0f);
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		if (m_bUIOverlayEnabled)
		{
			// Draw stuff here.
			Matrix4x4 matUIProj(1.0f);
			m_pShaderSystem->SetShaderUniform(m_hUIShader, 0, 0, 1, &matProjection);
			m_pShaderSystem->BindShader(m_hUIShader);
			m_pShaderSystem->BindPerInstanceUniforms(1);
			m_pRenderer->DrawGeometry(m_TestText.m_Geometry, m_pUIVertexBuffer, m_pUIIndexBuffer);
		}
	}
	m_pRenderer->EndRenderpass();
}

// Factory function to create the view.
IRenderView* RenderViewCreate_UIView()
{
	CUIRenderView* pNewView = new CUIRenderView;
	return pNewView;
}

// Key-Event handler.
static bool Event_KeyReleased(void* pEventListener, EventData_t data)
{
	CUIRenderView* pUIView = static_cast<CUIRenderView*>(pEventListener);
	char keycode = *data.c;

	if (pUIView && keycode == 'Y')
	{
		pUIView->m_bUIOverlayEnabled = !pUIView->m_bUIOverlayEnabled;
	}

	return true;
}
