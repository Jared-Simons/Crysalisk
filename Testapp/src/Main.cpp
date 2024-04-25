extern void printInt(int x);

#include <Core/Logging.h>
#include <Systems/IEngine.h>

int main()
{
	Engine->EngineInitialize();

	int x = 42;
	printInt(x);

	while (true)
	{
		Engine->EngineRun();
	}

	return 0;
}