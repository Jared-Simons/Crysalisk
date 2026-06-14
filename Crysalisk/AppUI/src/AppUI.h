//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Entry point to the AppUI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

namespace AppUI
{
	class IAppUI
	{
	public:
		virtual bool	Init() = 0;
	};
} // namespace AppUI

extern AppUI::IAppUI* g_pUISystem;

extern "C" APPUI_API AppUI::IAppUI* GetUIPluginInterface();
