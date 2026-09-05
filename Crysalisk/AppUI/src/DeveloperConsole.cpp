//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2026
// 
// Purpose: Developer console.
//=============================================================================
#include "DeveloperConsole.h"

// Core
#include <Core/Assertions.h>
#include <Core/ConVar.H>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Strings/String_Utils.h>

// Engine
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Input.h>

// Console event handlers.
static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data);
static bool ConsoleEvent_WindowResized(void* pListener, EventData_t data);
static void ConsoleEvent_MessageReceived(void* pConsole, const char* pszMessage);
static void ConsoleEvent_OnSubmitButtonPressed(void* pListenerInst);

// Mutex for multi-threaded logging.
// NOTE: This assumes only one console!
static Platform::ISystemMutex* s_pConsoleMutex = nullptr;

bool ConsoleCreate(AppUI::IAppUI* pUISystem, DeveloperConsole* pOutConsole)
{
	CASSERTMSG(pUISystem && pOutConsole, "ConsoleCreate() requires a pointer to a console and AppUI system!");
	CASSERTMSG(!s_pConsoleMutex, "Dev console currently only supports one instance!");

	// Background panel.
	pOutConsole->hBackgroundPanel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	pUISystem->SetControlPosition(pOutConsole->hBackgroundPanel, 0, 0);
	pUISystem->SetControlDimensions(pOutConsole->hBackgroundPanel, 1080, 180);
	pUISystem->SetControlColor(pOutConsole->hBackgroundPanel, 76, 76, 76, 215);

	// Readout panel.
	pOutConsole->hReadoutPanel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	pUISystem->SetControlParent(pOutConsole->hReadoutPanel, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hReadoutPanel, 100, 0);
	pUISystem->SetControlDimensions(pOutConsole->hReadoutPanel, 880, 130);
	pUISystem->SetControlColor(pOutConsole->hReadoutPanel, 50, 50, 50, 215);

	// Command submit button.
	pOutConsole->hCommandSubmitButton = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
	pUISystem->SetControlParent(pOutConsole->hCommandSubmitButton, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hCommandSubmitButton, 980, 130);
	pUISystem->SetControlDimensions(pOutConsole->hCommandSubmitButton, 60, 50);
	pUISystem->SetControlColor(pOutConsole->hCommandSubmitButton, 50, 50, 50, 215);
	pUISystem->SetControlPressedCallback(pOutConsole->hCommandSubmitButton, ConsoleEvent_OnSubmitButtonPressed, pOutConsole);

	// Command submit button label.
	pOutConsole->hCommandSubmitButtonLabel = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hCommandSubmitButtonLabel, pOutConsole->hCommandSubmitButton);
	pUISystem->SetControlPosition(pOutConsole->hCommandSubmitButtonLabel, 0, 35);
	pUISystem->SetControlFont(pOutConsole->hCommandSubmitButtonLabel, "UbuntuMono-R.ttf", 20.0f);
	pUISystem->SetControlText(pOutConsole->hCommandSubmitButtonLabel, "SUBMIT");

	// Label for console readout.
	pOutConsole->hConsoleReadout = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hConsoleReadout, pOutConsole->hReadoutPanel);
	pUISystem->SetControlPosition(pOutConsole->hConsoleReadout, 0, 75);
	pUISystem->SetControlFont(pOutConsole->hConsoleReadout, "UbuntuMono-R.ttf", 20.0f);
	pUISystem->SetControlText(pOutConsole->hConsoleReadout, "This is the readout!");

	// Text entry control.
	pOutConsole->hTextEntryControl = pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	pUISystem->SetControlParent(pOutConsole->hTextEntryControl, pOutConsole->hBackgroundPanel);
	pUISystem->SetControlPosition(pOutConsole->hTextEntryControl, 0, 170);
	pUISystem->SetControlFont(pOutConsole->hTextEntryControl, "UbuntuMono-R.ttf", 20.0f);
	pUISystem->SetControlText(pOutConsole->hTextEntryControl, "");
	pUISystem->SetControlInteractable(pOutConsole->hTextEntryControl, true);

	// Register for events.
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::INPUT_KEY_PRESSED, pOutConsole, ConsoleEvent_KeyPressed);
	pEventSystem->EventRegister(eEventCategory::WINDOW_RESIZE, pOutConsole, ConsoleEvent_WindowResized);
	pOutConsole->pUIContext = pUISystem;

	// Register this console with the engine's logger.
	g_pLog->RegisterConsoleWriteCallback(pOutConsole, ConsoleEvent_MessageReceived);

	// Default to the console being disabled.
	SetConsoleVisible(pUISystem, pOutConsole, false);

	// Setup mutex for multithreaded logging.
	s_pConsoleMutex = Platform::CreateSystemMutex();

	// Store off ConVar system state.
	pOutConsole->pConVar = g_pEngine->GetEngineInterfaces()->pConVar;

	LOG_DEBUG("Console Initialized");

	return true;
}

void ConsoleDestroy(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole)
{
	CASSERTMSG(pUISystem && pConsole, "ConsoleDestroy() requires a pointer to a console and AppUI system!");

	Platform::DestroySystemMutex(s_pConsoleMutex);
	s_pConsoleMutex = nullptr;

	g_pLog->UnregisterConsoleWriteCallback(pConsole);

	// Unregister for events.
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::WINDOW_RESIZE, pConsole, ConsoleEvent_WindowResized);
	pEventSystem->EventUnregister(eEventCategory::INPUT_KEY_PRESSED, pConsole, ConsoleEvent_KeyPressed);
}

void ConsoleUpdate(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole)
{
	CASSERTMSG(pUISystem && pConsole, "ConsoleUpdate() requires a pointer to a console and AppUI system!");
	if (pConsole->pendingStrings.Count() > 0)
	{
		Utl::CVector<char*>* pConsoleReadoutStrings = &pConsole->readoutStrings;

		for (u32 pMsg = 0; pMsg < pConsole->pendingStrings.Count(); pMsg++)
		{
			pConsoleReadoutStrings->PushBack(pConsole->pendingStrings.Data()[pMsg]);
		}

		// FIXME: Custom allocator?
		pConsole->pendingStrings.Clear();
		
		char szOutBuffer[10000]{};	// FIXME: Higher limit?

		u32 iLines = IntMin((u32)pConsoleReadoutStrings->Count(), 6);

		// Regenerate the readout text.
		if (pConsoleReadoutStrings->Count() > 1)
		{
			for (u32 iMessage = (u32)pConsoleReadoutStrings->Count() - iLines; iMessage < pConsoleReadoutStrings->Count(); iMessage++)
			{
				Utl::StringFormat_V("%s\n%s", szOutBuffer, szOutBuffer, pConsoleReadoutStrings->Data()[iMessage]);
			}
		}

		pConsole->pUIContext->SetControlText(pConsole->hConsoleReadout, szOutBuffer);
		pConsole->pUIContext->SetControlPosition(pConsole->hConsoleReadout, 0, IntMax(75 - (30 * iLines), 0));
	}
}

void SetConsoleVisible(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole, bool visible)
{
	CASSERTMSG(pUISystem && pConsole, "SetConsoleVisible() requires a pointer to a console and AppUI system!");

	pUISystem->SetControlVisible(pConsole->hBackgroundPanel, visible);
	pUISystem->SetControlVisible(pConsole->hCommandSubmitButton, visible);
	pUISystem->SetControlVisible(pConsole->hCommandSubmitButtonLabel, visible);
	pUISystem->SetControlVisible(pConsole->hReadoutPanel, visible);
	pUISystem->SetControlVisible(pConsole->hConsoleReadout, visible);
	pUISystem->SetControlVisible(pConsole->hTextEntryControl, visible);
	pUISystem->SetControlInteractable(pConsole->hTextEntryControl, visible);

	pConsole->visible = visible;

	// Set the active control
	if (visible)
	{
		// Set the background panel and force it to regenerate the layout so that the whole console
		// draws on top of everything else.
		pUISystem->SetActiveControl(pConsole->hBackgroundPanel, true);

		// Then set the text control to allow it to recieve key inputs without regenerating the layout.
		pUISystem->SetActiveControl(pConsole->hTextEntryControl, false);

		// Give the console render priority.
		pUISystem->SetRenderPriorityControl(pConsole->hBackgroundPanel);
	}
	else
	{
		pUISystem->SetActiveControl(kInvalidID_U32, true);
	}
}

void ConsoleSubmitCommand(AppUI::IAppUI* pUISystem, DeveloperConsole* pConsole)
{
	char* pCurrentEntryText = pConsole->pUIContext->GetControlText(pConsole->hTextEntryControl);
	char* pszTrimmedEntry = Utl::StringTrim(pCurrentEntryText);

	u64 length = Utl::StringLength(pszTrimmedEntry);
	if (length == 0 || (length == 1 && pszTrimmedEntry[0] == ' '))
	{
		goto cleanup_and_exit;
	}

	// Send it to the logger.
	LOG_INFO("%s", pszTrimmedEntry);

	// Attempt to execute the command.
	ConsoleCommandExecute(pConsole->pConVar, pszTrimmedEntry);

	// Save off the entry.
	pConsole->entryHistory.PushBack(Utl::StringDuplicate(pszTrimmedEntry));

	// Clear out the text entry control.
	// FIXME: We need to be able to clear the text without needing a space in there.
	pUISystem->SetControlText(pConsole->hTextEntryControl, "");

cleanup_and_exit:
	if (pszTrimmedEntry)
	{
		Utl::StringFree(pszTrimmedEntry);
		pszTrimmedEntry = nullptr;
	}
}

static bool ConsoleEvent_KeyPressed(void* pListener, EventData_t data)
{
	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;

	// Only process if the console is enabled.
	if (!pConsole->visible)
	{
		return false;
	}

	char c = data.c[0];

	if (c == KEY_ESCAPE || c == KEY_SHIFT || c == KEY_TILDE)
	{
		// Ignore certian keys.
		return false;
	}

	if (c == KEY_UP)
	{
		// Scroll through submitted messages.
		if (pConsole->entryHistory.Count() < 1)
		{
			return true;
		}

		pConsole->currentEntryIndex = (pConsole->currentEntryIndex + 1) % pConsole->entryHistory.Count();
		pConsole->pUIContext->SetControlText(pConsole->hTextEntryControl, pConsole->entryHistory[pConsole->currentEntryIndex]);

		return true;
	}
	else if (c == KEY_DOWN)
	{
		// Scroll the other way.
		if (pConsole->entryHistory.Count() < 1)
		{
			return true;
		}

		if ((i32)(pConsole->currentEntryIndex - 1) < 0)	// Handle wraparound
		{
			pConsole->currentEntryIndex = pConsole->entryHistory.Count() - 1;
		}
		else
		{
			pConsole->currentEntryIndex -= 1;
		}
		pConsole->pUIContext->SetControlText(pConsole->hTextEntryControl, pConsole->entryHistory[pConsole->currentEntryIndex]);

		return true;
	}

	// Get the current text in the control.
	char* pCurrentEntryText = pConsole->pUIContext->GetControlText(pConsole->hTextEntryControl);

	u64 length = Utl::StringLength(pCurrentEntryText);

	if (length < 1)
	{
		return false;
	}

	if (c == KEY_ENTER)
	{
		ConsoleSubmitCommand(pConsole->pUIContext, pConsole);
		return false;
	}

	// Let other listeners get this.
	return false;
}

static bool ConsoleEvent_WindowResized(void* pListener, EventData_t data)
{
	// Resize the elements so that they are proportional (the panel should stretch across the whole screen).
	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;
	AppUI::IAppUI* pUISystem = pConsole->pUIContext;

	u32 width = data.u32[0];
	u32 height = data.u32[1];

	pUISystem->SetControlDimensions(pConsole->hBackgroundPanel, width, (u16)(height * 0.25f));
	pUISystem->SetControlPosition(pConsole->hReadoutPanel, (u16)(width * 0.09f), 0);
	pUISystem->SetControlDimensions(pConsole->hReadoutPanel, (u16)(width * 0.81f), (u16)(height * 0.18f));
	pUISystem->SetControlPosition(pConsole->hCommandSubmitButton, (u16)(width * 0.90f), (u16)(height * 0.18f));
	pUISystem->SetControlPosition(pConsole->hCommandSubmitButtonLabel, 0, 35);
	pUISystem->SetControlPosition(pConsole->hTextEntryControl, 0, (u16)(height * 0.24f));

	// Let other listeners get this.
	return false;
}

static void ConsoleEvent_MessageReceived(void* pListener, const char* pszMessage)
{
	// Dump if our mutex isn't ready.
	if (!s_pConsoleMutex)
	{
		return;
	}

	// Ignore really long messages.
	if ((u32)Utl::StringLength(pszMessage) >= 512)
	{
		return;
	}

	DeveloperConsole* pConsole = (DeveloperConsole*)pListener;
	s_pConsoleMutex->Lock();
	pConsole->pendingStrings.PushBack(Utl::StringDuplicate(pszMessage));
	s_pConsoleMutex->Unlock();
}

static void ConsoleEvent_OnSubmitButtonPressed(void* pListenerInst)
{
	DeveloperConsole* pConsole = (DeveloperConsole*)pListenerInst;
	ConsoleSubmitCommand(pConsole->pUIContext, pConsole);
}