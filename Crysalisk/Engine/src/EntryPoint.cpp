//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The entry point to the application.
//=============================================================================
#include <Defines.h>

#include "Core/Engine.h"

//-------------------------------------------------------------------------
// Purpose: The application entry point.
//-------------------------------------------------------------------------
DLL_EXPORT int main()
{
	// TODO: This should be handled by the application config.
	EngineConfig_t engineConfig;
	engineConfig.isHeadless = false;

	if (!g_pEngine->Initialize(engineConfig))
	{
		return -1;
	}
	g_pEngine->Run();

	return 0;
}