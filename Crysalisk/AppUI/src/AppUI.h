//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Entry point to the AppUI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

// Forward declarations
struct ApplicationConfig_t;

namespace AppUI
{
	class IAppUI
	{
	public:
		virtual bool	Initialize(struct ::ApplicationConfig_t* pAppConfig) = 0;
	};
} // namespace AppUI

extern AppUI::IAppUI* g_pUISystem;

extern "C" APPUI_API AppUI::IAppUI* GetUIPluginInterface();
