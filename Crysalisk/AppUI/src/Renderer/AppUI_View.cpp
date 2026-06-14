//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Render view for the AppUI plugin.
//=============================================================================
#include "AppUI_View.h"

namespace AppUI
{
	class CAppUI_RenderView : public IRenderView
	{
	public:
		virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
		virtual void	Shutdown(IRenderer* pRenderer) override;

		virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) override;
		virtual void	Render(RenderPacket_t* pRenderData) override;

		virtual const char* GetViewName() const override { return "App UI View"; }
		virtual const Renderpass* GetRenderpass() const override { return &m_Renderpass; }

	private:
		Renderpass	m_Renderpass{};
	};

	bool CAppUI_RenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
	{
		return true;
	}

	void CAppUI_RenderView::Shutdown(IRenderer* pRenderer)
	{
	}

	void CAppUI_RenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
	{
	}

	void CAppUI_RenderView::Render(RenderPacket_t* pRenderData)
	{
	}

} // namespace AppUI

IRenderView* RenderViewCreate_AppUIView()
{
	return new AppUI::CAppUI_RenderView;
}
