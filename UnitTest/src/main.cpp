#include <Systems/IEngine.h>
#include <Core/Logging.h>
#include <Clock.h>

bool PerformUnitTests()
{
	LOG_DEBUG("Hello UnitTest!");
	
	return true;
}

int main()
{
	// Running unit tests in headless mode as we do not require rendering or input.
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = true;

	if (!Engine->Engine_Initialize(engineConfig))
		return -1;

	PerformUnitTests();

	Engine->Engine_Shutdown();

	return 0;
}