//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The entry point to the engine.
//=============================================================================
#include <Defines.h>

#include "Application.h"
#include "Core/Engine.h"

DLL_EXPORT int Crysalisk_Main(ApplicationConfig_t& pAppConfig);

//-------------------------------------------------------------------------
// Purpose: The engine entry point.
//-------------------------------------------------------------------------
int Crysalisk_Main(ApplicationConfig_t& pAppConfig)
{
	EngineConfig_t engineConfig;
	engineConfig.isHeadless = false;
	engineConfig.pAppConfig = &pAppConfig;

	if (!g_pEngine->Initialize(engineConfig))
	{
		return -1;
	}
	g_pEngine->Run();

	return 0;
}
