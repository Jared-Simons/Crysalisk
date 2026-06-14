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
struct RenderPacket_t;

namespace AppUI
{
	class IAppUI
	{
	public:
		virtual bool	PreInitialize(struct ApplicationConfig_t* pAppConfig) = 0; 
		virtual bool	Initialize(struct ApplicationConfig_t* pConfig) = 0;

		virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
	};
} // namespace AppUI

extern AppUI::IAppUI* g_pUISystem;

extern "C" APPUI_API AppUI::IAppUI* GetUIPluginInterface();
