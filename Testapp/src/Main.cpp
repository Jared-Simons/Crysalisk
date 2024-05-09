#include <Systems/IEngine.h>

int main()
{
	if (!Engine->Engine_Initialize())
		return -1;

	while (Engine->Engine_Run())
	{

	}

	return 0;
}