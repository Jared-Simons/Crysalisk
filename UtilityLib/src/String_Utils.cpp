//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "String_Utils.h"

#include <stdio.h>
#include <string>

char* StringFormat(const char* format, const char* str1, const char* str2)
{
	char out_string[10000];
	sprintf(out_string, format, str1, str2);
	return out_string;
}

char* StringFormat_V(const char* format, const char* str1, va_list& var_args)
{
	char out_string[10000];
	vsprintf(out_string, format, var_args);
	return out_string;
}

bool StringCompare(const char* str1, const char* str2)
{
	int result = strcmp(str1, str2);
	return result == 0;
}

int String_ATOI(const char c)
{
	int x;
	x = c;
	return x;
}
