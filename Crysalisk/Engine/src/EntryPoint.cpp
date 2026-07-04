//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The entry point to the engine.
//=============================================================================
#include <Defines.h>

#include "Application.h"
#include "Core/Engine.h"

DLL_EXPORT i32 Crysalisk_Main(ApplicationConfig_t& pAppConfig);

//-------------------------------------------------------------------------
// Purpose: The engine entry point.
//-------------------------------------------------------------------------
i32 Crysalisk_Main(ApplicationConfig_t& pAppConfig)
{
	EngineConfig_t engineConfig;
	engineConfig.isHeadless = false;
	engineConfig.pAppConfig = &pAppConfig;

	if (!pAppConfig.pEngine->Initialize(engineConfig))
	{
		return -1;
	}
	pAppConfig.pEngine->Run();

	return 0;
}
