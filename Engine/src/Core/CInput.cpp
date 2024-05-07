//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Implements the Core Input System.
//=============================================================================
#include "Systems/IInput.h"

#include "Core/Logging.h"
#include "Systems/IEvent.h"


class CInputSystem : public IInputSystem
{
public:
	CInputSystem() : m_KeysState{ 0 }, m_PrevKeysState{ 0 } {}

	bool		InputSystem_Initialize() override;
	void		InputSystem_Shutdown() override;
	void		InputSystem_Update() override;

	void		ProcessKeyInput(const int processedKeyInput, bool pressed) override;
	void		ProcessMouseInput(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags, bool pressed) override;

private:
	KeyState_T	m_KeysState[256];
	KeyState_T	m_PrevKeysState[256];

	MouseState_t m_MouseState;
};

bool CInputSystem::InputSystem_Initialize()
{
	// setup m_KeysState table
	for (char i = 0; i < MAX_INPUT_KEYS; i++)
	{
		m_KeysState[i].m_Char = i + 65;	// +65 so that array 0 lines up with 'A'
		m_KeysState[i].m_Pressed = false;

		m_PrevKeysState[i].m_Char = i + 65;
		m_PrevKeysState[i].m_Pressed = false;
	}

	// setup m_MouseState table
	m_MouseState.m_XPos = 0.0f;
	m_MouseState.m_YPos = 0.0f;
	for (int i = 0; i < MAX_MOUSE_BUTTONS / 2; i++)
	{
		m_MouseState.m_MouseButtons[i] = false;
	}

	LOG_INFO("Input System Initialized!");

	return true;
}

void CInputSystem::InputSystem_Shutdown()
{
	LOG_INFO("Input System Shutting Down");
}

void CInputSystem::InputSystem_Update()
{
	// TODO: Is this the cleanest/fastest way to handle this?
	for (char i = 0; i < MAX_INPUT_KEYS; i++)
	{
		m_PrevKeysState[i].m_Pressed = m_KeysState[i].m_Pressed;
	}
}

void CInputSystem::ProcessKeyInput(const int processedKeyInput, bool pressed)
{
	if (processedKeyInput != 0)
	{
		if (pressed)
		{
			m_KeysState[processedKeyInput - 65].m_Pressed = true;
			gpEngineEvents->EventFire(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, (void*)processedKeyInput);
		}
		else
		{
			m_KeysState[processedKeyInput - 65].m_Pressed = false;
			gpEngineEvents->EventFire(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, (void*)processedKeyInput);
		}
	}
	else
	{
		LOG_ERROR("Input System: Tried to process undefined keycode!");
	}
}

void CInputSystem::ProcessMouseInput(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags, bool pressed)
{
	if (mouseXPos != 0 && mouseYPos != 0)
	{
		m_MouseState.m_XPos = mouseXPos;
		m_MouseState.m_YPos = mouseYPos;

		//LOG_DEBUG("Mouse Pos: X:%f Y:%f", mouseXPos, mouseYPos);
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

		gpEngineEvents->EventFire(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, (void*)mouseButtonFlags);
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

		gpEngineEvents->EventFire(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, (void*)mouseButtonFlags);
	}
}

static CInputSystem sInputSystem;
IInputSystem* gpInputSystem = &sInputSystem;
