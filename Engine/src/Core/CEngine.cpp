//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "CEngine.h"

#include "Core/Logging.h"

bool CEngine::EngineInitialize()
{
	LoggingInitialize();
	
	LOG_INFO("EngineInitialize() called!\n");

	return true;
}

bool CEngine::EngineRun()
{
	//LOG_INFO("EngineRun() called!\n");

	return true;
}

void CEngine::EngineShutdown()
{
	LOG_INFO("EngineShutdown() called!\n");
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;