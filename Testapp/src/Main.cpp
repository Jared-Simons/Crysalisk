#include <Systems/IEngine.h>

int main()
{
	Engine->EngineInitialize();

	while (true)
	{
		Engine->EngineRun();
	}

	return 0;
}