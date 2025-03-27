//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#pragma once

#include <Defines.h>

DLL_EXPORT size_t StringLength(const char* str);

DLL_EXPORT void StringFormat(const char* format, const char* str1, const char* str2, char* outString);

DLL_EXPORT void StringFormat_V(const char* format, char* outString, ...);

DLL_EXPORT char* StringDuplicate(const char* pStringToCopy);
DLL_EXPORT char* StringDuplicate_N(const char* pStrToCopy, int bytesToCopy);

DLL_EXPORT void StringFree(char* pString);

DLL_EXPORT bool StringCompare(const char* str1, const char* str2);

DLL_EXPORT size_t StringIndexOf(const char* pString, char pCharToFind);

// Appends str2 to string 1.
DLL_EXPORT char* StringConcat(char* str1, char* str2);

DLL_EXPORT int StringTrim(char* pStr, char* pOutString);

// Strips all instances of 'c' from pStr.
// NOTE: This must be freed by the caller.
DLL_EXPORT char* StringStrip(char* pStr, char c);

DLL_EXPORT int String_ATOI(const char* c);
DLL_EXPORT float String_ATOF(const char* c);
