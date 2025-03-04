//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#pragma once

#include <Defines.h>

// TODO: Does this need to be included here? Would much rather have this in the .cpp file...
#include <stdarg.h>

DLL_EXPORT size_t StringLength(const char* str);

DLL_EXPORT void StringFormat(const char* format, const char* str1, const char* str2, char* outString);

DLL_EXPORT void StringFormat_V(const char* format, const char* str1, char* outString, ...);

DLL_EXPORT char* StringDuplicate(const char* pStringToCopy);

DLL_EXPORT void StringFree(char* pString);

DLL_EXPORT bool StringCompare(const char* str1, const char* str2);

// Appends str2 to string 1.
DLL_EXPORT char* StringConcat(char* str1, char* str2);

DLL_EXPORT i32 String_ATOI(const char c);

