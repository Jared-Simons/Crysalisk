#include <Defines.h>

#include "Core/Engine.h"

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