#include <stdio.h>

extern "C"
{
	__declspec(dllexport) bool Tools_Init();
	__declspec(dllexport) bool Tools_Run();
}

bool Tools_Init()
{
	printf("Welcome to Crysalisk Level Editor!\n");

	return true;
}

bool Tools_Run()
{
	printf("Running Level Editor Loop!\n");

	return true;
}
