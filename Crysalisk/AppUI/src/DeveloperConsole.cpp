//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2026
// 
// Purpose: Developer console.
//=============================================================================
#include "DeveloperConsole.h"

// Core
#include <Core/Assertions.h>
#include <Core/Logging.h>
#include <Strings/String_Utils.h>

// Engine
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Input.h>

// Console event handlers.
static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data);
static void ConsoleEvent_MessageReceived(void* pConsole, const char* pszMessage);

bool ConsoleCreate(AppUI::IAppUI* pUISystem, DeveloperConsole* pOutConsole)
{
	CASSERTMSG(pUISystem && pOutConsole, "ConsoleCreate() requires a pointer to a console and AppUI system!");

	// Background panel.
	pOutConsole->hBackgroundPanel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	pUISystem->SetControlPosition(pOutConsole->hBackgroundPanel, 0, 0);
	pUISystem->SetControlDimensions(pOutConsole->hBackgroundPanel, 1080, 180);	// FIXME: Make it so that this control DOES resize proportionately when the window resizes! The width of the panel should always cover the surface.
	pUISystem->SetControlColor(pOutConsole->hBackgroundPanel, 76, 76, 76, 215);

	// Readout panel.
	pOutConsole->hReadoutPanel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	pUISystem->SetControlParent(pOutConsole->hReadoutPanel, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hReadoutPanel, 100, 0);
	pUISystem->SetControlDimensions(pOutConsole->hReadoutPanel, 880, 130);
	pUISystem->SetControlColor(pOutConsole->hReadoutPanel, 50, 50, 50, 215);

	// Command submit button.
	// FIXME: If this happens later (After text?) then the color is messed up.
	pOutConsole->hCommandSubmitButton = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
	pUISystem->SetControlParent(pOutConsole->hCommandSubmitButton, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hCommandSubmitButton, 980, 130);
	pUISystem->SetControlDimensions(pOutConsole->hCommandSubmitButton, 50, 50);
	pUISystem->SetControlColor(pOutConsole->hCommandSubmitButton, 50, 50, 50, 215);

	// Label for console readout.
	pOutConsole->hConsoleReadout = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hConsoleReadout, pOutConsole->hReadoutPanel);
	pUISystem->SetControlPosition(pOutConsole->hConsoleReadout, 0, 75);
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

	// Register this system with the engine's logger.
	g_pLog->RegisterConsoleWriteCallback(pOutConsole, ConsoleEvent_MessageReceived);

	// Default to the console being disabled.
	SetConsoleVisible(pUISystem, pOutConsole, false);

	LOG_DEBUG("Console Initialized");

	return true;
}

void ConsoleDestroy(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole)
{
	CASSERTMSG(pUISystem && pConsole, "ConsoleDestroy() requires a pointer to a console and AppUI system!");

	g_pLog->UnregisterConsoleWriteCallback(pConsole);

	// Unregister for events.
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventUnregister(eEventCategory::INPUT_KEY_PRESSED, pConsole, ConsoleEvent_KeyPressed);

	// Command submit button.
	pUISystem->DestroyUIControl(pConsole->hCommandSubmitButton);
	pConsole->hCommandSubmitButton = kInvalidID_U32;

	// Console readout label.
	pUISystem->DestroyUIControl(pConsole->hConsoleReadout);
	pConsole->hConsoleReadout = kInvalidID_U32;

	// Background panel.
	pUISystem->DestroyUIControl(pConsole->hBackgroundPanel);
	pConsole->hBackgroundPanel = kInvalidID_U32;
}

void SetConsoleVisible(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole, bool bVisible)
{
	CASSERTMSG(pUISystem && pConsole, "SetConsoleVisible() requires a pointer to a console and AppUI system!");

	pUISystem->SetControlVisible(pConsole->hBackgroundPanel, bVisible);
	pUISystem->SetControlVisible(pConsole->hCommandSubmitButton, bVisible);
	pUISystem->SetControlVisible(pConsole->hReadoutPanel, bVisible);
	pUISystem->SetControlVisible(pConsole->hConsoleReadout, bVisible);
	pUISystem->SetControlVisible(pConsole->hTextEntryControl, bVisible);

	pConsole->bVisible = bVisible;
}

static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data)
{
	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;

	// Only process if the console is enabled.
	if (!pConsole->bVisible)
	{
		return false;
	}

	char c = data.c[0];

	if (c == KEY_ESCAPE || c == KEY_SHIFT || c == KEY_TILDE)
	{
		// Ignore certian keys.
		return false;
	}

	// Get the current text in the control.
	char* pCurrentEntryText = pConsole->pUIContext->GetControlText(pConsole->hTextEntryControl);

	if (c == KEY_ENTER)
	{
		// Attempt to execute the command (LEFTOFF: Implement)
		// Also send it to the logger.
		LOG_INFO("%s", pCurrentEntryText);

		// Clear out the text entry control.
		// FIXME: We need to be able to clear the text without needing a space in there.
		pConsole->pUIContext->SetControlText(pConsole->hTextEntryControl, "");

		return false;
	}

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

		// Check shift key state.
		bool bShiftPressed = g_pEngine->GetEngineInterfaces()->pInputSystem->IsKeyPressed(KEY_SHIFT);

		if (!bShiftPressed)
		{
			if (c >= KEY_A && c <= KEY_Z)
			{
				// Lowercase letters if necessary.
				c += ('a' - 'A');
			}
		}
		else
		{
			if (c >= KEY_0 && c <= KEY_9)
			{
				// Numbers/symbol case.
				// Map the key to the correct symbol.
				switch (c)
				{
				case KEY_0:
					c = 41;
					break;
				case KEY_1:
					c = 33;
					break;
				case KEY_2:
					c = 64;
					break;
				case KEY_3:
					c = 35;
					break;
				case KEY_4:
					c = 36;
					break;
				case KEY_5:
					c = 37;
					break;
				case KEY_6:
					c = 94;
					break;
				case KEY_7:
					c = 38;
					break;
				case KEY_8:
					c = 42;
					break;
				case KEY_9:
					c = 40;
					break;
				default:
					CAssert(0);
				}
			}
		}

		// Append the new char to it.
		Utl::StringFormat_V("%s%c", szOutBuffer, pCurrentEntryText, c);
	}

	// Apply to the control.
	pConsole->pUIContext->SetControlText(pConsole->hTextEntryControl, szOutBuffer);

	// Let other listeners get this.
	return false;
}

static void ConsoleEvent_MessageReceived(void* pListener, const char* pszMessage)
{
	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;

	pConsole->readoutStrings.PushBack(Utl::StringDuplicate(pszMessage));

	char szOutBuffer[10000]{};	// FIXME: Higher limit?
	Utl::StringFormat_V("%s", szOutBuffer, pConsole->readoutStrings.Back());

	// Regenerate the readout text.
	if (pConsole->readoutStrings.Count() > 1)
	{
		u8 messagesWritten{ 0 };
		for (u32 iMessage = (u32)pConsole->readoutStrings.Count() - 2; iMessage > 0 && messagesWritten < 10; iMessage--)
		{
			Utl::StringFormat_V("%s\n%s", szOutBuffer, szOutBuffer, pConsole->readoutStrings[iMessage]);
			messagesWritten++;
		}
	}

	pConsole->pUIContext->SetControlText(pConsole->hConsoleReadout, szOutBuffer);
}