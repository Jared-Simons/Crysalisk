//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Implements the Core Input System.
//=============================================================================
#include "Core/Input.h"

#include "Core/Event.h"

// Core
#include <Containers/CStack.h>
#include <Core/Logging.h>

//-------------------------------------------------------------------------
// Engine input system implementation.
//-------------------------------------------------------------------------
class CInputSystem : public IInputSystem
{
public:
	CInputSystem() : m_KeysState{ 0 }, m_PrevKeysState{ 0 }, m_KeyStateDirty{ true } {}

	// Initialization, shutdown.
	bool		Initialize(Platform::IPlatformWindow& pEngineWindow, IEventSystem* pEngineEventHandler) override;
	void		Shutdown() override;
	void		Update() override;

	virtual void	PushInputMap(InputMap_t* pInputMap) override;
	virtual void	PopInputMap() override;

	// Input processing.
	void		ProcessKeyInput(const i32 processedKeyInput, bool pressed) override;
	void		ProcessMouseButtonInput(byte mouseButtonFlags, bool pressed) override;
	void		ProcessMousePositionInput(const i32 mouseXPos, const i32 mouseYPos) override;

	virtual bool	IsKeyPressed(const i32 key) const override;
	virtual bool	IsMouseButtonPressed(eMouseButtonTable button) const override;
	virtual bool	WasMouseButtonPressed(eMouseButtonTable button) const override;
	virtual void	GetCurrentMousePos(i32& iX, i32& iY) override;

private:
	// Keyboard state this frame.
	KeyState_t	m_KeysState[256];
	// Keyboard state from the previous frame.
	KeyState_t	m_PrevKeysState[256];

	// Current mouse state.
	MouseState_t m_MouseState;
	// Mouse state from the previous frame.
	MouseState_t m_PrevMouseState;

	// Should the keys state be updated? (Was a key pressed this frame?)
	bool		m_KeyStateDirty;

	// Holds a pointer to the engine's event system.
	IEventSystem*	m_pEventSystem{ nullptr };

	// Input handler stack.
	Utl::CStack<InputMap_t*> m_InputHandlers{};
};

// Internal pointer to input system.
// Needed for API calls.
// TODO: maybe there's a better way to pass this around?
static CInputSystem* s_InputSystem{ nullptr };

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IInputSystem::AllocateInputSystemState(IInputSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold input system state was not provided! Input system creation failed!");
		return;
	}

	s_InputSystem = new CInputSystem;
	*ppSystemState = s_InputSystem;
}

//-------------------------------------------------------------------------
// Purpose - Initializes the input system.
// Input - none
// Output - bool: Returns true on success; otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::Initialize(Platform::IPlatformWindow& pEngineWindow, IEventSystem* pEngineEventHandler)
{
	if (!pEngineEventHandler)
	{
		LOG_ERROR("Input system was not provided with an event handler. Input system initialize failed!");
		return false;
	}
	m_pEventSystem = pEngineEventHandler;

	// setup m_KeysState table
	for (i32 i = 0; i < MAX_INPUT_KEYS; i++)
	{
		m_KeysState[i].m_Char = i + 65;	// +65 so that array 0 lines up with 'A'
		m_KeysState[i].m_bPressed = false;

		m_PrevKeysState[i].m_Char = i + 65;
		m_PrevKeysState[i].m_bPressed = false;
	}

	// setup m_MouseState table
	m_MouseState.m_XPos = 0;
	m_MouseState.m_YPos = 0;
	m_MouseState.m_bitsMouseButtons = 0;

	// Register our callbacks with the platform layer.
	pEngineWindow.SetKeyboardEventCallback(API_ProcessKeyInput);
	pEngineWindow.SetMouseButtonEventCallback(API_ProcessMouseButtonInput);
	pEngineWindow.SetMouseMoveEventCallback(API_ProcessMouseMoveInput);

	// Allocate memory for input stacks.
	m_InputHandlers.Resize(2);

	LOG_INFO("Input System Initialized!");

	return true;
}

//-------------------------------------------------------------------------
// Purpose - Shuts down the input system.
//-------------------------------------------------------------------------
void CInputSystem::Shutdown()
{
	m_InputHandlers.Clear();
	s_InputSystem = nullptr;
}

//-------------------------------------------------------------------------
// Purpose - Updates the internal state of the input system.
//			 Typically called once per application update.
//-------------------------------------------------------------------------
void CInputSystem::Update()
{
	if (m_KeyStateDirty)
	{
		Platform::MemoryCopy(m_PrevKeysState, m_KeysState, sizeof(KeyState_t));
		m_KeyStateDirty = false;
	}
	Platform::MemoryCopy(&m_PrevMouseState, &m_MouseState, sizeof(MouseState_t));
}

//-------------------------------------------------------------------------
// Purpose - Processes a key input passed in from some data stream.
//			 Also responsible for firing an event to any listeners.
// Input - processedKeyInput: The ordinal value of the key being processed.
//		   pressed: True if the key was pressed, otherwise false.
// Output - void.
//-------------------------------------------------------------------------
void CInputSystem::ProcessKeyInput(const i32 processedKeyInput, bool pressed)
{
	if (processedKeyInput != static_cast<i32>(eKeyTable::KEY_UNDEFINED))
	{
		EventData_t eventData;
		eventData.c[0] = processedKeyInput;

		bool stopEvent = false;

		if (pressed)
		{
			eventData.c[0] = processedKeyInput;
			bool repeat = m_PrevKeysState[processedKeyInput].m_bPressed;
			m_KeysState[processedKeyInput].m_bPressed = true;

			// Iterate through the input handler in case we need to go through the whole stack.
			for (i32 i = (i32)m_InputHandlers.Count() - 1; i >= 0 && !stopEvent; i--)
			{
				const InputMap_t* pCurMap = m_InputHandlers.Data()[i];
				stopEvent = pCurMap->PFN_KeyEvent_Callback(pCurMap->pListenerInst, (u16)processedKeyInput, pressed, repeat ? KEY_MESSAGE_FLAG_REPEATING : 0);
			}

			if (!stopEvent)
			{
				m_pEventSystem->EventFire(eEventCategory::INPUT_KEY_PRESSED, nullptr, eventData);
			}
		}
		else
		{
			m_KeysState[processedKeyInput].m_bPressed = false;

			// Iterate through the input handler in case we need to go through the whole stack.
			for (i32 i = (i32)m_InputHandlers.Count() - 1; i >= 0 && !stopEvent; i--)
			{
				const InputMap_t* pCurMap = m_InputHandlers.Data()[i];
				stopEvent = pCurMap->PFN_KeyEvent_Callback(pCurMap->pListenerInst, (u16)processedKeyInput, pressed, 0);
			}

			if (!stopEvent)
			{
				m_pEventSystem->EventFire(eEventCategory::INPUT_KEY_RELEASED, nullptr, eventData);
			}
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
void CInputSystem::ProcessMouseButtonInput(byte mouseButtonFlags, bool pressed)
{
	if (pressed)
	{
		// FIXME: make these actual flags.
		m_MouseState.m_bitsMouseButtons = mouseButtonFlags;

		EventData_t eventData;
		eventData.u32[0] = mouseButtonFlags;

		bool stopEvent = false;

		// Iterate through the input handler in case we need to go through the whole stack.
		for (i32 i = (i32)m_InputHandlers.Count() - 1; i >= 0 && !stopEvent; i--)
		{
			const InputMap_t* pCurMap = m_InputHandlers.Data()[i];
			stopEvent = pCurMap->PFN_MouseButtonEvent_Callback(pCurMap->pListenerInst, (u16)mouseButtonFlags);
		}

		if (!stopEvent)
		{
			m_pEventSystem->EventFire(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, eventData);
		}
	}
	else
	{
		// Unset button bit.
		m_MouseState.m_bitsMouseButtons &= ~(mouseButtonFlags);

		EventData_t eventData;
		eventData.u32[0] = mouseButtonFlags;

		bool stopEvent = false;

		// Iterate through the input handler in case we need to go through the whole stack.
		for (i32 i = (i32)m_InputHandlers.Count() - 1; i >= 0 && !stopEvent; i--)
		{
			const InputMap_t* pCurMap = m_InputHandlers.Data()[i];
			stopEvent = pCurMap->PFN_MouseButtonEvent_Callback(pCurMap->pListenerInst, (u16)mouseButtonFlags);
		}

		if (!stopEvent)
		{
			m_pEventSystem->EventFire(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, eventData);
		}
	}
}

void CInputSystem::ProcessMousePositionInput(const i32 mouseXPos, const i32 mouseYPos)
{
	m_MouseState.m_XPos = mouseXPos;
	m_MouseState.m_YPos = mouseYPos;

	// Fire off an event.
	EventData_t eventData;
	eventData.i32[0] = mouseXPos;
	eventData.i32[1] = mouseYPos;

	m_pEventSystem->EventFire(eEventCategory::INPUT_MOUSE_MOVED, nullptr, eventData);

	// If a mouse button is held down, fire a dragged event as well.
	if (m_MouseState.m_bitsMouseButtons != 0)
	{
		m_pEventSystem->EventFire(eEventCategory::INPUT_MOUSE_DRAGGED, nullptr, eventData);
	}
}

//-------------------------------------------------------------------------
// Purpose TODO: Documentation.
//-------------------------------------------------------------------------
void CInputSystem::PushInputMap(InputMap_t* pInputMap)
{
	CAssert(pInputMap);
	m_InputHandlers.Push(pInputMap);
}

//-------------------------------------------------------------------------
// Purpose TODO: Documentation.
//-------------------------------------------------------------------------
void CInputSystem::PopInputMap()
{
	m_InputHandlers.Pop();
}

//-------------------------------------------------------------------------
// Purpose - Checks to see if 'key' is currently pressed.
// Input - key: The ordinal value of the key to check for.
// Output - True if the key is currently pressed, otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::IsKeyPressed(const i32 key) const
{
	if (m_KeysState[key].m_bPressed)
		return true;

	return false;
}

//-------------------------------------------------------------------------
// Purpose - Checks to see if 'button' is currently pressed.
// Input - button: The mouse button value to check for.
// Output - True if the button is currently pressed, otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::IsMouseButtonPressed(eMouseButtonTable button) const
{
	if (m_MouseState.m_bitsMouseButtons & button)
	{
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------
// Purpose - Checks to see if 'button' was pressed in the previous frame.
// Input - button: The mouse button value to check for.
// Output - True if the button is currently pressed, otherwise false.
//-------------------------------------------------------------------------
bool CInputSystem::WasMouseButtonPressed(eMouseButtonTable button) const
{
	if ((m_MouseState.m_bitsMouseButtons & button) == 0 &&
		(m_PrevMouseState.m_bitsMouseButtons & button) != 0)
	{
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------
// Purpose - Returns the raster-space coordinates of the mouse.
// Input - iX/iY - References to ints that store the current mouse coordinates.
// Output - none.
//-------------------------------------------------------------------------
void CInputSystem::GetCurrentMousePos(i32& iX, i32& iY)
{
	iX = m_MouseState.m_XPos;
	iY = m_MouseState.m_YPos;
}

//-----------------------------------------------------------------------------
// API for external libraries to access our callbacks.
//-----------------------------------------------------------------------------
void API_ProcessKeyInput(i32 processedKeyInput, bool pressed) 
{ 
	s_InputSystem->ProcessKeyInput(processedKeyInput, pressed);
}

void API_ProcessMouseButtonInput(u32 mouseButtonFlags, bool bPressed)
{
	s_InputSystem->ProcessMouseButtonInput(mouseButtonFlags, bPressed);
}

void API_ProcessMouseMoveInput(const i32 mouseXPos, const i32 mouseYPos)
{
	s_InputSystem->ProcessMousePositionInput(mouseXPos, mouseYPos);
}