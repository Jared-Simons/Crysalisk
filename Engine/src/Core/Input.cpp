//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Implements the Core Input System.
//=============================================================================
#include "Core/Input.h"

#include "Core/Logging.h"
#include "Core/Event.h"
#include "Core/Memory.h"
#include "Core/Engine.h"


class CInputSystem : public IInputSystem
{
public:
	CInputSystem() : m_KeysState{ 0 }, m_PrevKeysState{ 0 }, m_KeyStateDirty{ true } {}

	void* operator new(size_t size, LinearAllocator* pAllocator)
	{
		void* block = pAllocator->Allocate(size);
		if (block)
		{
			return block;
		}
		else
		{
			LOG_FATAL("Systems Allocator failed to allocate memory for the input system!");
			return nullptr;
		}
	}

	// NOTE: This does nothing and just exists to clear a compiler warning, 
	// the memory was allocated and will be freed when the linear allocator gets cleared.
	// If the creation of this system fails, the engine will shutdown regardless.
	void	operator delete(void* pMemory, LinearAllocator* pAllocator) {}

	bool		Initialize() override;
	void		Shutdown() override;
	void		Update() override;

	void		ProcessKeyInput(const i32 processedKeyInput, bool pressed, Engine* enginePtr) override;
	void		ProcessMouseInput(const f32 mouseXPos, const f32 mouseYPos, u8 mouseButtonFlags, bool pressed) override;

	bool		IsKeyPressed(const i32 key) const override;

private:
	KeyState_T	m_KeysState[256];
	KeyState_T	m_PrevKeysState[256];

	MouseState_t m_MouseState;

	bool		m_KeyStateDirty;
};

static CInputSystem* sInputSystem{ nullptr };
IInputSystem* gpInputSystem = sInputSystem;

//-----------------------------------------------------------------------------
// Factory method to initialize an event system.
//-----------------------------------------------------------------------------
IInputSystem* IInputSystem::InputSystem_Initialize(LinearAllocator* pAllocator)
{
	if (pAllocator)
	{
		CInputSystem* inputSystem = new (pAllocator) CInputSystem();

		if (!inputSystem->Initialize())
		{
			return nullptr;
		}

		return inputSystem;
	}

	LOG_ERROR("Engine subsystem initialization requires a memory allocator!");
	return false;
}

bool CInputSystem::Initialize()
{
	// setup m_KeysState table
	for (u8 i = 0; i < MAX_INPUT_KEYS; i++)
	{
		m_KeysState[i].m_Char = i + 65;	// +65 so that array 0 lines up with 'A'
		m_KeysState[i].m_Pressed = false;

		m_PrevKeysState[i].m_Char = i + 65;
		m_PrevKeysState[i].m_Pressed = false;
	}

	// setup m_MouseState table
	m_MouseState.m_XPos = 0.0f;
	m_MouseState.m_YPos = 0.0f;
	for (u8 i = 0; i < MAX_MOUSE_BUTTONS / 2; i++)
	{
		m_MouseState.m_MouseButtons[i] = false;
	}

	LOG_INFO("Input System Initialized!");

	return true;
}

void CInputSystem::Shutdown()
{
	LOG_INFO("Input System Shutting Down");
}

void CInputSystem::Update()
{
	if (m_KeyStateDirty)
	{
		gpMemory->MemCopy(m_PrevKeysState, m_KeysState, sizeof(KeyState_T));
		m_KeyStateDirty = false;
	}
}

void CInputSystem::ProcessKeyInput(const i32 processedKeyInput, bool pressed, Engine* enginePtr)
{
	if (processedKeyInput != 0)
	{
		EventData_t eventData;
		eventData.c[0] = processedKeyInput;
		if (pressed)
		{
			eventData.c[0] = processedKeyInput;
			m_KeysState[processedKeyInput].m_Pressed = true;
			gpEvents->EventFire(eEventCategory::INPUT_KEY_PRESSED, nullptr, eventData);
		}
		else
		{
			m_KeysState[processedKeyInput].m_Pressed = false;
			gpEvents->EventFire(eEventCategory::INPUT_KEY_RELEASED, nullptr, eventData);
		}
	}
	else
	{
		LOG_ERROR("Input System: Tried to process undefined keycode!");
	}

	m_KeyStateDirty = true;
}

void CInputSystem::ProcessMouseInput(const f32 mouseXPos, const f32 mouseYPos, u8 mouseButtonFlags, bool pressed)
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

		EventData_t eventData;
		eventData.u16[0] = mouseButtonFlags;

		gpEvents->EventFire(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, eventData);
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

		gpEvents->EventFire(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, eventData);
	}
}

bool CInputSystem::IsKeyPressed(const i32 key) const
{
	if (m_KeysState[key].m_Pressed)
		return true;

	return false;
}
