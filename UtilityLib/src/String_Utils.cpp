//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "String_Utils.h"

#include <Systems/IMemory.h> // TODO: TEMP

#include <stdio.h>
#include <string>

size_t StringLength(const char* str)
{
	return strlen(str);
}

void StringFormat(const char* format, const char* str1, const char* str2, char* outString)
{
	char stringBuffer[10000];
	sprintf_s(stringBuffer, format, str1, str2);
	strcpy_s(outString, 10000, stringBuffer);
}

void StringFormat_V(const char* format, const char* str1, char* outString, ...)
{
	char stringBuffer[10000];
	va_list varArgs;
	va_start(varArgs, outString);
	vsnprintf_s(stringBuffer, 10000, format, varArgs);
	va_end(varArgs);
	//strcpy_s(outString, 10000, stringBuffer);
	strcpy(outString, stringBuffer);
}

bool StringCompare(const char* str1, const char* str2)
{
	int result = strcmp(str1, str2);
	return result == 0;
}

char* StringDuplicate(const char* pStrToCopy)
{
	size_t strLength = StringLength(pStrToCopy);
	char* stringCopy = (char*)gpMemorySystem->MemAllocate(strLength + 1, eMemAllocCategories::MEM_CATEGORY_STRING); // TODO: REPLACE WITH MEMORY ALLOC HOOKS FOR UTILLIB
	memcpy(stringCopy, pStrToCopy, strLength + 1);

	// NOTE: The caller is responsible for freeing this memory.
	return stringCopy;
}

void StringConcat(char* str1, const char* str2)
{
	size_t len = StringLength(str1) + StringLength(str2);
	strcat_s(str1, len + 1, str2);
}

int String_ATOI(const char c)
{
	int x;
	x = c;
	return x;
}
