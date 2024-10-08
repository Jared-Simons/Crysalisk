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

size_t StringLength(const char* str);

void StringFormat(const char* format, const char* str1, const char* str2, char* outString);

void StringFormat_V(const char* format, const char* str1, char* outString, ...);

char* StringDuplicate(const char* pStringToCopy);

bool StringCompare(const char* str1, const char* str2);

// Appends str2 to string 1.
void StringConcat(char* str1, const char* str2);

i32 String_ATOI(const char c);

