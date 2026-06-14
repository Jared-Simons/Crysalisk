//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Entry point to the AppUI plugin.
//=============================================================================
#include "AppUI.h"

#include "Renderer/AppUI_View.h"

// Core
#include <Core/Logging.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Systems/ShaderSystem.h>

// FIXME: Control factory?
#include "Controls/Panel.h"

namespace AppUI
{
	BaseUIControl* pPanelTest = nullptr;

	class CAppUI : public IAppUI
	{
	public:
		virtual bool	PreInitialize(struct ApplicationConfig_t* pAppConfig) override;
		virtual bool	Initialize(struct ApplicationConfig_t* pConfig) override;

		virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;

	private:
		// Engine system pointers.
		IRenderer*		m_pRenderer{ nullptr };
		IShaderSystem*	m_pShaderSystem{ nullptr };
		RenderBuffer*	m_pVertexBuffer{ nullptr };
		RenderBuffer*	m_pIndexBuffer{ nullptr };
	};

	bool CAppUI::PreInitialize(struct ApplicationConfig_t* pAppConfig)
	{
		CAssert(pAppConfig);

		pAppConfig->pViews.PushBack(RenderViewCreate_AppUIView());

		return true;
	}

	bool CAppUI::Initialize(struct ApplicationConfig_t* pAppConfig)
	{
		CAssert(pAppConfig);

		LOG_INFO("Initializing AppUI framework");

		m_pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;
		m_pShaderSystem = pAppConfig->pEngine->GetEngineInterfaces()->pShaderSystem;

		// Get renderbuffer pointers.
		// FIXME: This is bad, we need a better way of passing renderbuffers around for geometry.
		m_pVertexBuffer = m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
		m_pIndexBuffer = m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

		pPanelTest = CreatePanel(0, 0, 100, 100, m_pRenderer, m_pShaderSystem, m_pVertexBuffer, m_pIndexBuffer);

		return true;
	}

	void CAppUI::FramePrepare(struct RenderPacket_t* pRenderPacket)
	{
		pRenderPacket->pEditorGeo = &pPanelTest->m_Geometry;
	}

} // namespace AppUI

// FIXME: Store this somewhere, doesn't need to be global.
AppUI::CAppUI s_UISystem;
AppUI::IAppUI* g_pUISystem = &s_UISystem;

AppUI::IAppUI* GetUIPluginInterface()
{
	return g_pUISystem;
}
