//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2026
// 
// Purpose: Developer console.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Containers/CVector.h>

// AppUI
#include <AppUI.h>
#include <UI_Types.h>

// Callback for the log system to make to console listeners.
typedef void (*PFN_ConsoleEvent_MessageSent)(void* pConsole, const char* pszMessage);

struct DeveloperConsole
{
	AppUI::UIControlHandle_t hBackgroundPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hReadoutPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hConsoleReadout{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hTextEntryControl{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hCommandSubmitButton{ kInvalidID_U32 };

	// Lines of text to be rendered by the console.
	Utl::CVector<char*>		readoutStrings{};

	// Should the console be rendered?
	bool bVisible{ false };

	// A pointer to the UI system that creates/manages these controls.
	// TODO: This is only here so that we can access the system during event callbacks. This should probably be handled some other way.
	AppUI::IAppUI*	pUIContext{ nullptr };
};

DLL_EXPORT bool ConsoleCreate(AppUI::IAppUI* pUISystem, DeveloperConsole* pOutConsole);
DLL_EXPORT void ConsoleDestroy(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole);

DLL_EXPORT void SetConsoleVisible(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole, bool bVisible);