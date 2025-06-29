//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#pragma once

#include <Defines.h>

// TODO: Move these to a namespace.

// Returns the length of a string.
DLL_EXPORT size_t StringLength(const char* str);

// Attempts to process two strings and writes the results to 'outString'.
DLL_EXPORT void StringFormat(const char* format, const char* str1, const char* str2, char* outString);

// Attempts to perform variadic formatting on the provided data.
DLL_EXPORT void StringFormat_V(const char* format, char* outString, ...);

// Compares two strings.
DLL_EXPORT bool StringCompare(const char* str1, const char* str2);
// Case-insensitive string comparison.
DLL_EXPORT bool StringCompareInsensitive(const char* str1, const char* str2);

// Returns the index of 'charToFind' within 'pString', kInvalidID_U32 if not found.
DLL_EXPORT unsigned int StringIndexOf(const char* pString, char charToFind);

// Attempts to get the file extension given a file's path.
DLL_EXPORT char* StringGetFilenameExtension(const char* pString);

// String duplication.
DLL_EXPORT char* StringDuplicate(const char* pStringToCopy);
// String duplication up to 'bytesToCopy' chars.
DLL_EXPORT char* StringDuplicate_N(const char* pStrToCopy, size_t charsToCopy);

// Frees dynamically-allocated string data.
DLL_EXPORT void StringFree(char* pString);

// Appends pszStr1 to pszStr2.
DLL_EXPORT char* StringConcat(char* pszStr1, char* pszStr2);

// Removes the bordering whitespace from a string (NOTE: This performs a dynamic allocation).
DLL_EXPORT char* StringTrim(char* pStr);
// Removes the bordering whitespace from a string (NOTE: This does not perform a dynamic allocation).
DLL_EXPORT size_t StringTrimInPlace(char* pszStr, char* pOutString);

// Strips all instances of 'c' from pStr.
// NOTE: This must be freed by the caller.
DLL_EXPORT char* StringStrip(char* pszStr, char c);
DLL_EXPORT void StringStripInPlace(char* pStr, char c);

// Converts an ascii value to an integer.
DLL_EXPORT int String_ATOI(const char* c);
// Converts an ascii value to a floating-point value.
DLL_EXPORT float String_ATOF(const char* c);
