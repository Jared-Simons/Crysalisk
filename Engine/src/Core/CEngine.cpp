//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "CEngine.h"

#include "Core/Logging.h"
#include "Core/Memory.h"
#include "Systems/IEvent.h"
#include "Platform/Platform.h"

static bool Event_ApplicationQuit(void* data);
static bool s_bApplicationQuitCalled = false;

bool CEngine::Engine_Initialize()
{
	Logging_Initialize();
	LOG_INFO("EngineInitialize() called!");

	// Memory System
	gpMemorySystem->MemorySystem_Initialize();

	// Event System
	gpEngineEvents->EventSystem_Initialize();

	// Register for engine-level events
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);

	Platform_Initialize();

	return true;
}

bool CEngine::Engine_Run()
{
	while (!s_bApplicationQuitCalled)
	{
		//LOG_INFO("EngineRun() called!");

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

	// Unregister from engine-level events
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);

	gpEngineEvents->EventSystem_Shutdown();

	gpMemorySystem->MemorySystem_Shutdown();

	Logging_Shutdown();
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;


bool Event_ApplicationQuit(void* user)
{
	LOG_VERBOSE("Application_Quit() called");
	s_bApplicationQuitCalled = true;
	return true;
}