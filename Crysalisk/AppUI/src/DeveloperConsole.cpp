//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2026
// 
// Purpose: Developer console.
//=============================================================================
#include "DeveloperConsole.h"

// Core
#include <Core/Assertions.h>
#include <Strings/String_Utils.h>

// Engine
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/InputTypes.h>

// Console event handlers.
static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data);

bool ConsoleCreate(AppUI::IAppUI* pUISystem, DeveloperConsole* pOutConsole)
{
	CASSERTMSG(pUISystem && pOutConsole, "ConsoleCreate() requires a pointer to a console and AppUI system!");

	// Background panel.
	pOutConsole->hBackgroundPanel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	pUISystem->SetControlPosition(pOutConsole->hBackgroundPanel, 0, 0);
	pUISystem->SetControlDimensions(pOutConsole->hBackgroundPanel, 1080, 180);	// FIXME: Make it so that this control DOES resize proportionately when the window resizes! The width of the panel should always cover the surface.
	pUISystem->SetControlColor(pOutConsole->hBackgroundPanel, 76, 76, 76, 215);

	// Label for console readout.
	pOutConsole->hConsoleReadout = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hConsoleReadout, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hConsoleReadout, 0, 100);
	pUISystem->SetControlFont(pOutConsole->hConsoleReadout, "Assets/Fonts/UbuntuMono-R.ttf", 20.0f);
	pUISystem->SetControlText(pOutConsole->hConsoleReadout, "This is the readout!");

	// Text entry control.
	pOutConsole->hTextEntryControl = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hTextEntryControl, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hTextEntryControl, 0, 170);
	pUISystem->SetControlFont(pOutConsole->hTextEntryControl, "Assets/Fonts/UbuntuMono-R.ttf", 20.0f);
	pUISystem->SetControlText(pOutConsole->hTextEntryControl, "");

	// Register for events.
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::INPUT_KEY_PRESSED, pOutConsole, ConsoleEvent_KeyPressed);
	pOutConsole->pUIContext = pUISystem;

	return true;
}

void ConsoleDestroy(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole)
{
	CASSERTMSG(pUISystem && pConsole, "ConsoleDestroy() requires a pointer to a console and AppUI system!");

	// Unregister for events.
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventUnregister(eEventCategory::INPUT_KEY_PRESSED, pConsole, ConsoleEvent_KeyPressed);

	// Console readout label.
	pUISystem->DestroyUIControl(pConsole->hConsoleReadout);
	pConsole->hConsoleReadout = kInvalidID_U32;

	// Background panel.
	pUISystem->DestroyUIControl(pConsole->hBackgroundPanel);
	pConsole->hBackgroundPanel = kInvalidID_U32;
}

static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data)
{
	char c = data.c[0];

	if (c == KEY_ESCAPE)
	{
		// Ignore escape keys.
		return false;
	}

	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;

	// Get the current text in the control.
	char* pCurrentEntryText = pConsole->pUIContext->GetControlText(pConsole->hTextEntryControl);
	char szOutBuffer[10000]{};

	// Handle special cases
	if (c == KEY_BACKSPACE)
	{
		// Take the current text, and remove one character from it.
		u64 length = Utl::StringLength(pCurrentEntryText);

		if (length < 1)
		{
			return false;
		}

		Platform::MemoryCopy(szOutBuffer, pCurrentEntryText, length - 1);
		szOutBuffer[length] = 0;
	}
	else
	{
		// Default case.

		// Append the new char to it.
		Utl::StringFormat_V("%s%c", szOutBuffer, pCurrentEntryText, c);
	}

	// Apply to the control.
	pConsole->pUIContext->SetControlText(pConsole->hTextEntryControl, szOutBuffer);

	// Let other listeners get this.
	return false;
}