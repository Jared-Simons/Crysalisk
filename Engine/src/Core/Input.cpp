//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Implements the Core Input System.
//=============================================================================
#include "Core/Input.h"

#include "Core/Engine.h"
#include "Core/Event.h"
#include "Core/Logging.h"
#include <Memory/MemoryManager.h>

//-------------------------------------------------------------------------
// Engine input system implementation.
//-------------------------------------------------------------------------
class CInputSystem : public IInputSystem
{
public:
	CInputSystem() : m_KeysState{ 0 }, m_PrevKeysState{ 0 }, m_KeyStateDirty{ true } {}

	// Initialization, shutdown.
	bool		Initialize(Platform::IPlatformWindow& pEngineWindow) override;
	void		Shutdown() override;
	void		Update() override;

	// Input processing.
	void		ProcessKeyInput(const int processedKeyInput, bool pressed) override;
	void		ProcessMouseInput(const float mouseXPos, const float mouseYPos, byte mouseButtonFlags, bool pressed) override;

	bool		IsKeyPressed(const int key) const override;

private:
	// Keyboard state this frame.
	KeyState_t	m_KeysState[256];
	// Keyboard state from the previous frame.
	KeyState_t	m_PrevKeysState[256];

	// Current mouse state.
	MouseState_t m_MouseState;

	// Should the keys state be updated? (Was a key pressed this frame?)
	bool		m_KeyStateDirty;
};

// Input system context.
static CInputSystem sInputSystem;
IInputSystem* g_pInputSystem_Internal = &sInputSystem;

//-------------------------------------------------------------------------
// Purpose - Initializes the input system.
// Input - none
// Output - bool: Returns true on success; otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::Initialize(Platform::IPlatformWindow& pEngineWindow)
{
	// setup m_KeysState table
	for (int i = 0; i < MAX_INPUT_KEYS; i++)
	{
		m_KeysState[i].m_Char = i + 65;	// +65 so that array 0 lines up with 'A'
		m_KeysState[i].m_bPressed = false;

		m_PrevKeysState[i].m_Char = i + 65;
		m_PrevKeysState[i].m_bPressed = false;
	}

	// setup m_MouseState table
	m_MouseState.m_XPos = 0.0f;
	m_MouseState.m_YPos = 0.0f;
	for (int i = 0; i < MAX_MOUSE_BUTTONS / 2; i++)
	{
		m_MouseState.m_MouseButtons[i] = false;
	}

	// Register our callbacks with the platform layer.
	pEngineWindow.SetKeyboardEventCallback(API_ProcessKeyInput);
	pEngineWindow.SetMouseEventCallback(API_ProcessMouseInput);

	LOG_INFO("Input System Initialized!");

	return true;
}

//-------------------------------------------------------------------------
// Purpose - Shuts down the input system.
//-------------------------------------------------------------------------
void CInputSystem::Shutdown()
{
}

//-------------------------------------------------------------------------
// Purpose - Updates the internal state of the input system.
//			 Typically called once per application update.
//-------------------------------------------------------------------------
void CInputSystem::Update()
{
	if (m_KeyStateDirty)
	{
		// FIXME: We need to be saving off mouse state as well...
		Platform::MemoryCopy(m_PrevKeysState, m_KeysState, sizeof(KeyState_t));
		m_KeyStateDirty = false;
	}
}

//-------------------------------------------------------------------------
// Purpose - Processes a key input passed in from some data stream.
//			 Also responsible for firing an event to any listeners.
// Input - processedKeyInput: The ordinal value of the key being processed.
//		   pressed: True if the key was pressed, otherwise false.
// Output - void.
//-------------------------------------------------------------------------
void CInputSystem::ProcessKeyInput(const int processedKeyInput, bool pressed)
{
	if (processedKeyInput != static_cast<int>(eKeyTable::KEY_UNDEFINED))
	{
		EventData_t eventData;
		eventData.c[0] = processedKeyInput;
		if (pressed)
		{
			eventData.c[0] = processedKeyInput;
			m_KeysState[processedKeyInput].m_bPressed = true;
			g_pEngineEvents_Internal->EventFire(eEventCategory::INPUT_KEY_PRESSED, nullptr, eventData);
		}
		else
		{
			m_KeysState[processedKeyInput].m_bPressed = false;
			g_pEngineEvents_Internal->EventFire(eEventCategory::INPUT_KEY_RELEASED, nullptr, eventData);
		}
	}
	else
	{
		LOG_ERROR("Input System: Tried to process undefined keycode!");
	}

	m_KeyStateDirty = true;
}

//-------------------------------------------------------------------------
// Purpose - Processes mouse input.
//			 Also responsible for firing an event to any listeners.
// Input - mouseXPos/mouseYPos: the X and Y positions of the cursor on the window.
//		   mouseButtonFlags: A bitfield of the mouse buttons to be processed.
//		   pressed: True if the processed buttons are pressed, otherwise false.
// Output - void.
//-------------------------------------------------------------------------
void CInputSystem::ProcessMouseInput(const float mouseXPos, const float mouseYPos, byte mouseButtonFlags, bool pressed)
{
	if (mouseXPos != 0 && mouseYPos != 0)
	{
		m_MouseState.m_XPos = mouseXPos;
		m_MouseState.m_YPos = mouseYPos;
		
		// Fire off an event.
		EventData_t eventData;
		eventData.f32[0] = mouseXPos;
		eventData.f32[1] = mouseYPos;

		g_pEngineEvents_Internal->EventFire(eEventCategory::INPUT_MOUSE_MOVED, nullptr, eventData);
	}

	if (mouseButtonFlags != 0 && pressed)
	{
		switch (mouseButtonFlags)
		{
		case 1:
			m_MouseState.m_MouseButtons[0] = true;
			break;

		case 3:
			m_MouseState.m_MouseButtons[1] = true;
			break;

		case 5:
			m_MouseState.m_MouseButtons[2] = true;
			break;

		default:
			LOG_ERROR("Error processing mouse button input!");
			break;
		}

		EventData_t eventData;
		eventData.u16[0] = mouseButtonFlags;

		g_pEngineEvents_Internal->EventFire(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, eventData);
	}

	if (mouseButtonFlags != 0 && !pressed)
	{
		switch (mouseButtonFlags)
		{
		case 2:
			m_MouseState.m_MouseButtons[0] = false;
			break;

		case 4:
			m_MouseState.m_MouseButtons[1] = false;
			break;

		case 6:
			m_MouseState.m_MouseButtons[2] = false;
			break;

		default:
			LOG_ERROR("Error processing mouse button input!");
			break;
		}

		EventData_t eventData;
		eventData.u16[0] = mouseButtonFlags;

		g_pEngineEvents_Internal->EventFire(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, eventData);
	}
}

//-------------------------------------------------------------------------
// Purpose - Checks to see if 'key' is currently pressed.
// Input - key: The ordinal value of the key to check for.
// Output - True if the key is currently pressed, otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::IsKeyPressed(const int key) const
{
	if (m_KeysState[key].m_bPressed)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// API for external libraries to access our callbacks.
//-----------------------------------------------------------------------------
void API_ProcessKeyInput(int processedKeyInput, bool pressed) 
{ 
	g_pInputSystem_Internal->ProcessKeyInput(processedKeyInput, pressed);
}

void API_ProcessMouseInput(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags)
{
	g_pInputSystem_Internal->ProcessMouseInput(mouseXPos, mouseYPos, mouseButtonFlags, false);
}