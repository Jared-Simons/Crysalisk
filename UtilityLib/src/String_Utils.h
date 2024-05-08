//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#pragma once

// TODO: Does this need to be included here? Would much rather have this in the .cpp file...
#include <stdarg.h>


char* StringFormat(const char* format, const char* str1, const char* str2);

char* StringFormat_V(const char* format, const char* str, va_list& var_args);

bool StringCompare(const char* str1, const char* str2);

int String_ATOI(const char c);

