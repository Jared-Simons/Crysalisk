//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "CEngine.h"

#include "Core/Logging.h"
#include "Core/Memory.h"
#include "Core/CEvent.h"
#include "Platform/Platform.h"

bool CEngine::Engine_Initialize()
{
	Logging_Initialize();
	LOG_INFO("EngineInitialize() called!");

	// Memory System
	gpMemorySystem->MemorySystem_Initialize();

	// Event System
	gpEngineEvents->EventSystem_Initialize();

	Platform_Initialize();

	return true;
}

bool CEngine::Engine_Run()
{
	//LOG_INFO("EngineRun() called!");

	Platform_Update();

	return true;
}

void CEngine::Engine_Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	gpEngineEvents->EventSystem_Shutdown();

	gpMemorySystem->MemorySystem_Shutdown();

	Logging_Shutdown();
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;

