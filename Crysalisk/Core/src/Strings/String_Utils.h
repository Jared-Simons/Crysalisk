//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#pragma once

#include <Defines.h>
#include "Math/MathLib.h"

namespace Utl
{
	// Returns the length of a string.
	DLL_EXPORT u64 StringLength(const char* str);

	// Attempts to process two strings and writes the results to 'outString'.
	DLL_EXPORT void StringFormat(const char* format, const char* str1, const char* str2, char* outString);

	// Attempts to perform variadic formatting on the provided data.
	DLL_EXPORT void StringFormat_V(const char* format, char* outString, ...);

	// Compares two strings.
	DLL_EXPORT bool StringCompare(const char* str1, const char* str2);
	// Case-insensitive string comparison.
	DLL_EXPORT bool StringCompareInsensitive(const char* str1, const char* str2);

	// Returns the index of 'charToFind' within 'pString', kInvalidID_U32 if not found.
	DLL_EXPORT u32 StringIndexOf(const char* pString, char charToFind);

	// Attempts to get the file extension given a file's path.
	DLL_EXPORT char* StringGetFilenameExtension(const char* pString);

	// Attempts to get the name of a file from its path.
	DLL_EXPORT char* StringGetFilenameFromPath(const char* pPath);

	// String duplication.
	DLL_EXPORT char* StringDuplicate(const char* pStringToCopy);
	// String duplication up to 'bytesToCopy' chars.
	DLL_EXPORT char* StringDuplicate_N(const char* pStrToCopy, u64 charsToCopy);

	// Frees dynamically-allocated string data.
	DLL_EXPORT void StringFree(char* pString);

	// Appends pszStr1 to pszStr2.
	DLL_EXPORT char* StringConcat(char* pszStr1, char* pszStr2);

	// Removes the bordering whitespace from a string (NOTE: This performs a dynamic allocation).
	DLL_EXPORT char* StringTrim(char* pStr);

	// Strips all instances of 'c' from pStr.
	// NOTE: This must be freed by the caller.
	DLL_EXPORT char* StringStrip(char* pszStr, char c);
	DLL_EXPORT void StringStripInPlace(char* pStr, char c);

	// Converts an ascii value to an integer.
	DLL_EXPORT i32 String_ATOI(const char* c);
	// Converts an ascii value to a floating-point value.
	DLL_EXPORT f32 String_ATOF(const char* c);

	// Converts an ascii string to a Vector3D_OLD.
	// NOTE: This function does not return the Vector so that the function is compatible with C-linkage.
	DLL_EXPORT void String_ToVector3(const char* c, Vector3D* pOutVec);

} // namespace Utl