#include <stdio.h>

__declspec(dllexport) void printInt(int x)
{
	printf("%d", x);
}