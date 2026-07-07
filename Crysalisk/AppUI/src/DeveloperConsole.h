//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2026
// 
// Purpose: Developer console.
//=============================================================================
#pragma once

#include <Defines.h>

// AppUI
#include <AppUI.h>
#include <UI_Types.h>

struct DeveloperConsole
{
	AppUI::UIControlHandle_t hBackgroundPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hConsoleReadout{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hTextEntryControl{ kInvalidID_U32 };

	// A pointer to the UI system that creates/manages these controls.
	// TODO: This is only here so that we can access the system during event callbacks. This should probably be handled some other way.
	AppUI::IAppUI*	pUIContext{ nullptr };
};

DLL_EXPORT bool ConsoleCreate(AppUI::IAppUI* pUISystem, DeveloperConsole* pOutConsole);
DLL_EXPORT void ConsoleDestroy(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole);