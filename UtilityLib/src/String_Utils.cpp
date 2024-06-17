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

void StringFormat_V(const char* format, const char* str1, va_list& var_args, char* outString)
{
	char stringBuffer[10000];
	vsprintf_s(stringBuffer, format, var_args);
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
