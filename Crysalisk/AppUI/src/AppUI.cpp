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

namespace AppUI
{
	class CAppUI : public IAppUI
	{
	public:
		virtual bool	Initialize(struct ::ApplicationConfig_t* pAppConfig) override;
	};

	bool CAppUI::Initialize(struct ::ApplicationConfig_t* pAppConfig)
	{
		LOG_INFO("Initializing AppUI framework");

		pAppConfig->pViews.PushBack(RenderViewCreate_AppUIView());

		return true;
	}

} // namespace AppUI

AppUI::CAppUI s_UISystem;
AppUI::IAppUI* g_pUISystem = &s_UISystem;

AppUI::IAppUI* GetUIPluginInterface()
{
	return g_pUISystem;
}
