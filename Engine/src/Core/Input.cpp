//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Implements the Core Input System.
//=============================================================================
#include "Core/Input.h"

#include "Core/Logging.h"
#include "Core/Event.h"
#include "Systems/IMemory.h"
#include "Systems/IEngine.h"


class CInputSystem : public IInputSystem
{
public:
	CInputSystem() : m_KeysState{ 0 }, m_PrevKeysState{ 0 }, m_KeyStateDirty{ true } {}

	void* operator new(size_t size, LinearAllocator* pAllocator)
	{
		void* block = pAllocator->allocate(size);
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

	bool		Initialize(IEventSystem* pEventSystem) override;
	void		Shutdown() override;
	void		Update() override;

	void		ProcessKeyInput(const int processedKeyInput, bool pressed, IEngine* enginePtr) override;
	void		ProcessMouseInput(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags, bool pressed) override;

	bool		IsKeyPressed(const int key) const override;

private:
	KeyState_T	m_KeysState[256];
	KeyState_T	m_PrevKeysState[256];

	MouseState_t m_MouseState;

	bool		m_KeyStateDirty;

private:
	// Hold a pointer to the engine's event system so we can fire off events.
	IEventSystem*	m_pEventSystem{ nullptr };
};

static CInputSystem* sInputSystem{ nullptr };
IInputSystem* gpInputSystem = sInputSystem;

//-----------------------------------------------------------------------------
// Factory method to initialize an event system.
//-----------------------------------------------------------------------------
IInputSystem* IInputSystem::InputSystem_Initialize(LinearAllocator* pAllocator, IEventSystem* pEventSystem)
{
	if (pAllocator)
	{
		CInputSystem* inputSystem = new (pAllocator) CInputSystem();

		if (!inputSystem->Initialize(pEventSystem))
		{
			return nullptr;
		}

		// HACK: This exposes the engine input system through a global accessor.
		// This is hacky and should be cleaned up down the line...
		sInputSystem = inputSystem;
		gpInputSystem = sInputSystem;

		return inputSystem;
	}

	LOG_ERROR("Engine subsystem initialization requires a memory allocator!");
	return false;
}

//-----------------------------------------------------------------------------
// Factory method to shutdown an input system.
//-----------------------------------------------------------------------------
void IInputSystem::InputSystem_Shutdown(IInputSystem* statePtr, bool bOwnsMemory)
{
	if (statePtr)
	{
		statePtr->Shutdown();

		if (bOwnsMemory)
		{
			delete statePtr;
			statePtr = nullptr;
		}
	}
}

bool CInputSystem::Initialize(IEventSystem* pEventSystem)
{
	// TODO: Logging for event system.

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

	m_pEventSystem = pEventSystem;

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
		gpMemorySystem->MemCopy(m_PrevKeysState, m_KeysState, sizeof(KeyState_T));
		m_KeyStateDirty = false;
	}
}

void CInputSystem::ProcessKeyInput(const int processedKeyInput, bool pressed, IEngine* enginePtr)
{
	if (processedKeyInput != 0)
	{
		EventData_t eventData;
		eventData.c[0] = processedKeyInput;
		if (pressed)
		{
			eventData.c[0] = processedKeyInput;
			m_KeysState[processedKeyInput].m_Pressed = true;
			EventSystem::EventFire(eEventCategory::INPUT_KEY_PRESSED, nullptr, eventData);
		}
		else
		{
			m_KeysState[processedKeyInput].m_Pressed = false;
			EventSystem::EventFire(eEventCategory::INPUT_KEY_RELEASED, nullptr, eventData);
		}
	}
	else
	{
		LOG_ERROR("Input System: Tried to process undefined keycode!");
	}

	m_KeyStateDirty = true;
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

		EventData_t eventData;
		eventData.u16[0] = mouseButtonFlags;

		EventSystem::EventFire(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, eventData);
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

		EventSystem::EventFire(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, eventData);
	}
}

bool CInputSystem::IsKeyPressed(const int key) const
{
	if (m_KeysState[key].m_Pressed)
		return true;

	return false;
}
