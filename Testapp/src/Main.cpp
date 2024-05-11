#include <Systems/IEngine.h>

int main()
{
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = false;

	if (!Engine->Engine_Initialize(engineConfig))
		return -1;

	while (Engine->Engine_Run())
	{

	}

	return 0;
}