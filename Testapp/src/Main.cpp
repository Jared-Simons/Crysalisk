#include <Systems/IEngine.h>

int main()
{
	Engine->Engine_Initialize();

	while (true)
	{
		Engine->Engine_Run();
	}

	return 0;
}