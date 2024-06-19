//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "String_Utils.h"

#include <stdio.h>
#include <string>

void StringFormat(const char* format, const char* str1, const char* str2, char* outString)
{
	char stringBuffer[10000];
	sprintf(stringBuffer, format, str1, str2);
	strcpy(outString, stringBuffer);
}

void StringFormat_V(const char* format, const char* str1, char* outString, ...)
{
	char stringBuffer[10000];
	va_list varArgs;
	va_start(varArgs, outString);
	vsprintf_s(stringBuffer, format, varArgs);
	va_end(varArgs);
	strcpy(outString, stringBuffer);
}

bool StringCompare(const char* str1, const char* str2)
{
	int result = strcmp(str1, str2);
	return result == 0;
}

void StringConcat(char* str1, const char* str2)
{
	strcat(str1, str2);
}

int String_ATOI(const char c)
{
	int x;
	x = c;
	return x;
}
