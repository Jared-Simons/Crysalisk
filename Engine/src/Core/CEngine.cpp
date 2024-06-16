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
#include "Systems/IRenderer.h"

static bool s_bApplicationQuitCalled = false;
static bool Event_ApplicationQuit(void* data);
static bool Event_KeyPressed(void* data);
static bool Event_KeyReleased(void* data);
static bool Event_MouseButtonPressed(void* data);
static bool Event_MouseButtonReleased(void* data);

class CEngine : public IEngine
{
public:
	bool	Engine_Initialize(EngineConfig_t engineConfig) override;

	bool	Engine_Run() override;

	void	Engine_Shutdown() override;

private:
	EngineConfig_t	m_engineConfig;
};

bool CEngine::Engine_Initialize(EngineConfig_t engineConfig)
{
	m_engineConfig = engineConfig;

	// Logging Utilities
	Logging_Initialize();

	// Memory System
	if (!gpMemorySystem->MemorySystem_Initialize())
	{
		LOG_FATAL("Memory System failed to initialize! Application cannot continue.");
		return false;
	}

	// Event System
	if (!gpEngineEvents->EventSystem_Initialize())
	{
		LOG_FATAL("Event System failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, Event_KeyPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, Event_KeyReleased);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, Event_MouseButtonPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, Event_MouseButtonReleased);


	// Input System
	if (!gpInputSystem->InputSystem_Initialize())
	{
		LOG_FATAL("Input System failed to initialize! Application cannot continue.");
		return false;
	}

	// Platform Layer
	if (!m_engineConfig.isHeadless)
	{
		if (!Platform_Initialize())
		{
			LOG_FATAL("Platform Layer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Renderer
	if (!m_engineConfig.isHeadless)
	{
		if (!gpRenderer->Renderer_Initialize())
		{
			LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	LOG_INFO("Engine Finished Initialization");
	return true;
}

bool CEngine::Engine_Run()
{
	while (!s_bApplicationQuitCalled)
	{
		if (!m_engineConfig.isHeadless)
		{
			gpInputSystem->InputSystem_Update();

			Platform_Update();

			if (!gpRenderer->Renderer_Update())
			{
				LOG_FATAL("Renderer update failed!");
				return false;
			}
		}

		return true;
	}

	Engine_Shutdown();

	// Returning false signifies that the engine is done running and ready for shutdown
	return false;
}

void CEngine::Engine_Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	if (!m_engineConfig.isHeadless)
	{
		gpRenderer->Renderer_Shutdown();
	}

	if (!m_engineConfig.isHeadless)
	{
		Platform_Shutdown();
	}

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

