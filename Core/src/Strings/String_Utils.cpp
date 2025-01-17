//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "Strings/String_Utils.h"

#include <Memory/MemoryManager.h>

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
	strcpy_s(outString, 10000, stringBuffer);
	//strcpy(outString, stringBuffer);
}

bool StringCompare(const char* str1, const char* str2)
{
	i32 result = strcmp(str1, str2);
	return result == 0;
}

char* StringDuplicate(const char* pStrToCopy)
{
	u64 strLength = StringLength(pStrToCopy);
	char* stringCopy = reinterpret_cast<char*>(g_pMemoryManager->Allocate(strLength + 1));
	memcpy(stringCopy, pStrToCopy, strLength + 1);

	// NOTE: The caller is responsible for freeing this memory.
	return stringCopy;
}

void StringFree(char* pString)
{
	if (pString)
	{
		u64 strLength = StringLength(pString);
		g_pMemoryManager->Free(reinterpret_cast<void*>(pString));
	}
}

void StringConcat(char* str1, const char* str2)
{
	size_t len = StringLength(str1) + StringLength(str2);
	strcat_s(str1, len + 1, str2);
}

i32 String_ATOI(const char c)
{
	i32 x;
	x = c;
	return x;
}
