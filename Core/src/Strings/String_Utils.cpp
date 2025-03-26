//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "Strings/String_Utils.h"

#include "Core/Logging.h"
#include <Memory/MemoryManager.h>

#include <stdio.h>
#include <string>
#include <stdarg.h>

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

void StringFormat_V(const char* format, char* outString, ...)
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

size_t StringIndexOf(const char* pString, char pCharToFind)
{
	size_t len = StringLength(pString);
	for (int idx = 0; idx < len; idx++)
	{
		if (pString[idx] == pCharToFind)
			return idx;
	}

	return kInvalidID_U32;
}

char* StringDuplicate(const char* pStrToCopy)
{
	u64 strLength = StringLength(pStrToCopy);
	char* stringCopy = reinterpret_cast<char*>(g_pMemoryManager->Allocate(strLength + 1));
	memcpy(stringCopy, pStrToCopy, strLength + 1);
	stringCopy[strLength] = 0;

	// NOTE: The caller is responsible for freeing this memory.
	return stringCopy;
}

char* StringDuplicate_N(const char* pStrToCopy, int bytesToCopy)
{
	char* stringCopy = new char[bytesToCopy + 1];
	memcpy(stringCopy, pStrToCopy, bytesToCopy + 1);
	stringCopy[bytesToCopy] = '\0';

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

char* StringConcat(char* str1, char* str2)
{
	size_t len1 = StringLength(str1);
	size_t len2 = StringLength(str2);

	// Make sure we're null-terminated.
	char* newString = new char[len1 + len2 + 1];
	strcpy(newString, str1);
	strcpy(newString + len1, str2);
	newString[len1 + len2] = '\0'; // Null terminator
	return newString;
}

char* StringStrip(char* pStr, char c)
{
	if (!pStr)
	{
		return nullptr;
	}

	size_t len = StringLength(pStr);
	size_t numInstances = 0;

	for (int idx = 0; idx < len; idx++)
	{
		if (pStr[idx] == c)
			numInstances++;
	}

	char* newString = new char[(len - numInstances) + 1];
	numInstances = 0;
	for (int idx = 0; idx < (len - numInstances); idx++)
	{
		if (pStr[idx] == c)
		{
			numInstances++;
			continue;
		}

		newString[idx - numInstances] = pStr[idx];
	}
	newString[(len - numInstances) - 1] = '\0';
	return newString;
}

int String_ATOI(const char* c)
{
	int result{ 0 };
	int curPower = 0;
	size_t stringLength = StringLength(c);

	for (int idx = 0; idx < stringLength; idx++)
	{
		result = result * 10 + (c[idx] - '0');
	}

	return result;
}
