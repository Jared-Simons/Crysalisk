//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "CEngine.h"

#include "Core/Logging.h"
#include "Core/Memory.h"
#include "Platform/Platform.h"

bool CEngine::Engine_Initialize()
{
	Logging_Initialize();
	LOG_INFO("EngineInitialize() called!");

	g_pMemorySystem->MemorySystem_Initialize();

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

	Logging_Shutdown();
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;