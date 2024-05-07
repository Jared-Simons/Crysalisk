//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "Systems/IEngine.h"

#include "Core/Logging.h"
#include "Systems/IMemory.h"
#include "Systems/IEvent.h"
#include "Systems/IInput.h"
#include "Platform/Platform.h"

static bool s_bApplicationQuitCalled = false;
static bool Event_ApplicationQuit(void* data);
static bool Event_KeyPressed(void* data);
static bool Event_KeyReleased(void* data);
static bool Event_MouseButtonPressed(void* data);
static bool Event_MouseButtonReleased(void* data);

class CEngine : public IEngine
{
public:
	bool	Engine_Initialize() override;

	bool	Engine_Run() override;

	void	Engine_Shutdown() override;

private:
};

bool CEngine::Engine_Initialize()
{
	Logging_Initialize();

	// Memory System
	gpMemorySystem->MemorySystem_Initialize();

	// Event System
	gpEngineEvents->EventSystem_Initialize();

	// Register for engine-level events
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, Event_KeyPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, Event_KeyReleased);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, Event_MouseButtonPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, Event_MouseButtonReleased);

	// Input System
	gpInputSystem->InputSystem_Initialize();

	Platform_Initialize();

	LOG_INFO("Engine Finished Initialization");
	return true;
}

bool CEngine::Engine_Run()
{
	while (!s_bApplicationQuitCalled)
	{
		gpInputSystem->InputSystem_Update();

		Platform_Update();

		return true;
	}

	Engine_Shutdown();

	// Returning false signifies that the engine is done running and ready for shutdown
	return false;
}

void CEngine::Engine_Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	gpInputSystem->InputSystem_Shutdown();

	// Unregister from engine-level events
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, Event_MouseButtonReleased);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, Event_MouseButtonPressed);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, Event_KeyReleased);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, Event_KeyPressed);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);

	gpEngineEvents->EventSystem_Shutdown();

	gpMemorySystem->MemorySystem_Shutdown();

	Logging_Shutdown();
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;


bool Event_ApplicationQuit(void* data)
{
	LOG_VERBOSE("Application_Quit() called");
	s_bApplicationQuitCalled = true;
	return true;
}

bool Event_KeyPressed(void* data)
{
	char keycode = (char)data;
	LOG_DEBUG("%c Key Pressed", keycode);
	return true;
}

bool Event_KeyReleased(void* data)
{
	char keycode = (char)data;
	LOG_DEBUG("%c Key Released", keycode);
	return true;
}

bool Event_MouseButtonPressed(void* data)
{
	unsigned int mouseButtonFlags = (unsigned int)data;
	LOG_DEBUG("%s Mouse Button Pressed", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}

bool Event_MouseButtonReleased(void* data)
{
	unsigned int mouseButtonFlags = (unsigned int)data;
	LOG_DEBUG("%s Mouse Button Released", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}

