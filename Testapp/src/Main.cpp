#include <Defines.h>
#include <Core/Engine.h>

i32 main()
{
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = false;

	Engine pEngineState;

	if (!Engine::Engine_Initialize(engineConfig, &pEngineState))
		return -1;

	while (pEngineState.Engine_Run())
	{

	}

	return 0;
}