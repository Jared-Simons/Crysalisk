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

namespace AppUI
{
	class CAppUI : public IAppUI
	{
	public:
		virtual bool	Init() override;

	private:
		IRenderView*	m_pAppUI_RenderView{ nullptr };
	};

	bool CAppUI::Init()
	{
		LOG_INFO("Initializing AppUI framework");

		m_pAppUI_RenderView = RenderViewCreate_AppUIView();

		return true;
	}

} // namespace AppUI

AppUI::CAppUI s_UISystem;
AppUI::IAppUI* g_pUISystem = &s_UISystem;

AppUI::IAppUI* GetUIPluginInterface()
{
	return g_pUISystem;
}
