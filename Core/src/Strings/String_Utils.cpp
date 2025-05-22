//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "Strings/String_Utils.h"

#include "Core/Logging.h"

#include <string>
#include <stdarg.h>

#pragma warning( push ) // TODO: Get rid of this
#pragma warning( disable: 4996 )

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

// NOTE: This only supports strings UP TO 512 characters!
// This had to be shortened because the larger string sizes were blowing out the stack on deep call stacks.
void StringFormat_V(const char* format, char* outString, ...)
{
	char stringBuffer[512];
	va_list varArgs;
	va_start(varArgs, outString);
	vsnprintf_s(stringBuffer, 512, format, varArgs);
	va_end(varArgs);
	strcpy_s(outString, 512, stringBuffer);
	//strcpy(outString, stringBuffer);
}

bool StringCompare(const char* str1, const char* str2)
{
	int result = strcmp(str1, str2);
	return result == 0;
}

unsigned int StringIndexOf(const char* pString, char pCharToFind)
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
	size_t strLength = StringLength(pStrToCopy);
	char* stringCopy = new char[strLength + 1];
	memcpy(stringCopy, pStrToCopy, strLength + 1);
	stringCopy[strLength] = 0;

	// NOTE: The caller is responsible for freeing this memory.
	return stringCopy;
}

char* StringDuplicate_N(const char* pStrToCopy, size_t bytesToCopy)
{
	char* stringCopy = new char[bytesToCopy + 1];
	memcpy(stringCopy, pStrToCopy, bytesToCopy + 1);
	stringCopy[bytesToCopy] = 0;

	// NOTE: The caller is responsible for freeing this memory.
	return stringCopy;
}

void StringFree(char* pString)
{
	if (pString)
	{
		delete[] pString;
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

char* StringTrim(char* pStr)
{
	size_t strLength = StringLength(pStr);

	size_t startIndex = -1;
	size_t endIndex = -1;
	size_t numWhitespaceCharacters = 0;

	for (int idx = 0; idx < strLength; idx++)
	{
		if (pStr[idx] != ' ' && startIndex == -1)
		{
			// Set the start index.
			startIndex = idx;
		}

		if ((pStr[idx] == ' ' || pStr[idx] == '\t') && startIndex != -1)
		{
			// Whitespace token found.
			numWhitespaceCharacters++;
		}

		if (startIndex != -1 && endIndex != -1)
			break;
	}

	if (endIndex == -1)
	{
		endIndex = strLength + 1;
	}

	size_t newStringLength = endIndex - startIndex - numWhitespaceCharacters + 1;
	size_t lastNonWhitespaceIdx{ 0 };
	char* pTrimmedString = new char[newStringLength];

	// Step through the string again, this time copying over all the characters BUT the whitespace characters.
	for (int idx = 0, oldIdx = 0; idx < newStringLength; idx++, oldIdx++)
	{
		// Ignore all whitespace tokens.
		while (pStr[oldIdx] == ' ' || pStr[oldIdx] == '\t')
			oldIdx++;

		pTrimmedString[idx] = pStr[oldIdx];
		lastNonWhitespaceIdx = idx;
	}

	pTrimmedString[lastNonWhitespaceIdx] = 0;
	return pTrimmedString;
}

size_t StringTrimInPlace(char* pStr, char* pOutString)
{
	size_t strLength = StringLength(pStr);

	size_t startIndex = -1;
	size_t endIndex = -1;
	size_t numWhitespaceCharacters = 0;

	for (int idx = 0; idx < strLength; idx++)
	{
		if (pStr[idx] != ' ' && startIndex == -1)
		{
			// Set the start index.
			startIndex = idx;
		}

		if ((pStr[idx] == ' ' || pStr[idx] == '\t' || pStr[idx] == 0) && startIndex != -1)
		{
			// Whitespace token found.
			numWhitespaceCharacters++;
		}

		if (startIndex != -1 && endIndex != -1)
			break;
	}

	if (endIndex == -1)
	{
		endIndex = strLength + 1;
	}

	size_t newStringLength = endIndex - startIndex - numWhitespaceCharacters + 1;

	// Step through the string again, this time copying over all the characters BUT the whitespace characters.
	for (int idx = 0, oldIdx = 0; idx < newStringLength; idx++, oldIdx++)
	{
		// Ignore all whitespace tokens.
		while (pStr[oldIdx] == ' ' || pStr[oldIdx] == '\t' || pStr[oldIdx] == 0)
			oldIdx++;

		pOutString[idx] = pStr[oldIdx];
	}

	pOutString[newStringLength - numWhitespaceCharacters] = 0;
	return newStringLength;
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

void StringStripInPlace(char* pStr, char c)
{
	if (!pStr)
	{
		return;
	}

	size_t len = StringLength(pStr);
	size_t numInstances = 0;

	for (int idx = 0; idx < len; idx++)
	{
		if (pStr[idx] == c)
		{
			// Strip this character and pull the rest of the string back.
			for (int inIdx = idx; inIdx < len; inIdx++)
			{
				pStr[inIdx] = pStr[inIdx + 1];
			}
		}
	}
}

int String_ATOI(const char* c)
{
	/*int result{0};
	int curPower = 0;
	size_t stringLength = StringLength(c);
	unsigned int dotIndex = StringIndexOf(c, '.');

	if (dotIndex != kInvalidID_U32)
		stringLength = dotIndex;

	for (int idx = 0; idx < stringLength; idx++)
	{
		result = result * 10 + (c[idx] - '0');
	}

	return result;*/ // TODO: Fix this!

	return atoi(c);
}

// TODO: Our own version of this would be nice.
float String_ATOF(const char* c)
{
	return static_cast<float>(atof(c));
}

#pragma warning( pop )