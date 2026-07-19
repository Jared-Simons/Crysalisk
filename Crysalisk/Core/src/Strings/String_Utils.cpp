//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Some utility string functions
//=============================================================================
#include "Strings/String_Utils.h"

#include "Core/Assertions.h"
#include "Core/Logging.h"

#include <string>
#include <stdarg.h>

namespace Utl
{
	//-------------------------------------------------------------------------
	// Purpose: Gets the length of a zero-terminated string.
	// 
	// Input:	pszString - The string's path to return.
	// Output:	i32 - The size of the string.
	//-------------------------------------------------------------------------
	u64 StringLength(const char* pszString)
	{
		if (!pszString)
			return 0;

		return strlen(pszString);
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to process two strings and writes the results to 'outString'.
	// 
	// Input:	pszFormat - The 'string's path to return.'format string'.
	//			str1 and str2 - The strings to use in parsing.
	//			pOutString - The string buffer to write to.
	// Output:	none.
	//-------------------------------------------------------------------------
	void StringFormat(const char* pszFormat, const char* pszStr1, const char* pszStr2, char* pOutString)
	{
		char stringBuffer[10000];
		sprintf_s(stringBuffer, pszFormat, pszStr1, pszStr2);
		strcpy_s(pOutString, 10000, stringBuffer);
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to perform variadic formatting on the provided data.
	// 
	// Input:	pszFormat - The 'string's path to return.'format string'.
	//			pOutString - The string buffer to write to.
	// Output:	none.
	//-------------------------------------------------------------------------
	void StringFormat_V(const char* pszFormat, char* pOutString, ...)
	{
		char stringBuffer[10000];
		va_list varArgs;
		va_start(varArgs, pOutString);
		i32 len = vsnprintf_s(stringBuffer, 10000, pszFormat, varArgs);
		va_end(varArgs);
		strcpy_s(pOutString, len + 1, stringBuffer);
	}

	//-------------------------------------------------------------------------
	// Purpose: Compares two strings.
	// 
	// Input:	pszStr1 & pszStr2 - The strings to be compared.
	// Output:	bool - True if the strings are equal, otherwise false.
	//-------------------------------------------------------------------------
	bool StringCompare(const char* pszStr1, const char* pszStr2)
	{
		if (!pszStr1 || !pszStr2)
		{
			CAssert(0);
			return false;
		}

		i32 result = strcmp(pszStr1, pszStr2);
		return result == 0;
	}

	//-------------------------------------------------------------------------
	// Purpose: Case-insensitive string comparison.
	// 
	// Input:	pszStr1 & pszStr2 - The strings to be compared.
	// Output:	bool - True if the strings are equal, otherwise false.
	//-------------------------------------------------------------------------
	bool StringCompareInsensitive(const char* pszStr1, const char* pszStr2)
	{
		i32 result = _strcmpi(pszStr1, pszStr2);
		return result == 0;
	}

	//-------------------------------------------------------------------------
	// Purpose: Returns the index of 'charToFind' within 'pString'.
	// 
	// Input:	pszString - The string to search.
	//			charToFind - The character to search for.
	// Output:	i32 - Returns the index of 'charToFind', or kInvalidID_U32 if not found.
	//-------------------------------------------------------------------------
	u32 StringIndexOf(const char* pszString, char charToFind)
	{
		u64 len = StringLength(pszString);
		for (i32 idx = 0; idx < len; idx++)
		{
			if (pszString[idx] == charToFind)
				return idx;
		}

		return kInvalidID_U32;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to get the file extension given a file's path.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszString - The string to get the extension from.
	// Output:	char* - A copy of the extension from pszString.
	//-------------------------------------------------------------------------
	char* StringGetFilenameExtension(const char* pszString)
	{
		u64 length = StringLength(pszString);
		u64 curIdx = length - 1;
		char* pCurChar = const_cast<char*>(&pszString[length - 1]);
		while (pCurChar && *pCurChar != '.')
		{
			pCurChar--;
			curIdx--;
		}

		char* pszExtension = StringDuplicate_N(&pszString[curIdx], length - curIdx);

		return pszExtension;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to get the name of a file from its path.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszPath - The path to get the name from.
	// Output:	char* - A copy of the name from pszPath.
	//-------------------------------------------------------------------------
	char* StringGetFilenameFromPath(const char* pszPath)
	{
		char*		pszName = nullptr;
		u32	stringLength = (u32)StringLength(pszPath);
		u32	iStartIdx = 0;
		u32	iEndIdx = 0;
		u32	iCurIdx = stringLength;
		const char* p = &pszPath[stringLength - 1];
		while (p)
		{
			if (*p == '.')
			{
				iEndIdx = iCurIdx;
			}

			if (*p == '/')	
			{
				iStartIdx = iCurIdx;
				break;
			}
		
			iCurIdx--;
			p--;
		}

		CAssert(iStartIdx < iEndIdx);
		pszName = StringDuplicate_N(&pszPath[iStartIdx], iEndIdx - iStartIdx - 1);
		return pszName;
	}

	//-------------------------------------------------------------------------
	// Purpose: String duplication.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszStrToCopy - The string to duplicate.
	// Output:	char* - A copy of pszString.
	//-------------------------------------------------------------------------
	char* StringDuplicate(const char* pszStrToCopy)
	{
		u64 strLength = StringLength(pszStrToCopy);
		char* stringCopy = new char[strLength + 1];
		memcpy(stringCopy, pszStrToCopy, strLength + 1);
		stringCopy[strLength] = 0;

		// NOTE: The caller is responsible for freeing this memory.
		return stringCopy;
	}

	//-------------------------------------------------------------------------
	// Purpose: String duplication up to 'bytesToCopy' chars.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszStrToCopy - The string to duplicate.
	//			bytesToCopy - The number of chars to copy.
	// Output:	char* - A copy of pszString.
	//-------------------------------------------------------------------------
	char* StringDuplicate_N(const char* pszStrToCopy, u64 charsToCopy)
	{
		char* stringCopy = new char[charsToCopy + 1];
		memcpy(stringCopy, pszStrToCopy, charsToCopy + 1);
		stringCopy[charsToCopy] = 0;

		// NOTE: The caller is responsible for freeing this memory.
		return stringCopy;
	}

	//-------------------------------------------------------------------------
	// Purpose: Frees dynamically-allocated string data.
	// 
	// Input:	pszString - The string to free.
	// Output:	none.
	//-------------------------------------------------------------------------
	void StringFree(char* pszString)
	{
		if (pszString)
		{
			delete[] pszString;
		}
	}

	//-------------------------------------------------------------------------
	// Purpose: Appends pszStr1 to pszStr2.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszStr1 & pszStr2 - The strings to be appended.
	// Output:	A new string created from pszStr1 & pszStr2.
	//-------------------------------------------------------------------------
	char* StringConcat(char* pszStr1, char* pszStr2)
	{
		u64 len1 = StringLength(pszStr1);
		u64 len2 = StringLength(pszStr2);

		// Make sure we're null-terminated.
		char* newString = new char[len1 + len2 + 1];
		char* pszStringPtr2 = &newString[len1];

		// NOTE: Not using Utl::StringDuplicate() because this method already allocates memory for us.
		Platform::MemoryCopy(newString, pszStr1, len1);
		Platform::MemoryCopy(pszStringPtr2, pszStr2, len2);

		newString[len1 + len2] = '\0'; // Null terminator
		return newString;
	}

	//-------------------------------------------------------------------------
	// Purpose: Removes the bordering whitespace from a string.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszStr - The string to be trimmed.
	// Output:	The trimmed string.
	//-------------------------------------------------------------------------
	char* StringTrim(char* pszStr)
	{
		u64 strLength = StringLength(pszStr);

		i64 startIndex = 0;
		i64 endIndex = -1;
		u64 numWhitespaceCharacters = 0;
		bool startingCharFound = false;

		for (i32 idx = 0; idx < strLength; idx++)
		{
			if ((pszStr[idx] == ' ' || pszStr[idx] == '\t') && startIndex != -1)
			{
				// Only count characters at the start and end of a string.
				if (startIndex != 0 && endIndex == -1)
				{
					// Whitespace token found.
					numWhitespaceCharacters++;
				}
			}
			endIndex++;

			if ((pszStr[idx] != ' ' && pszStr[idx] != '\t') && !startingCharFound)
			{
				// Set the start index.
				startIndex = idx;
				startingCharFound = true;
			}
		}

		if (endIndex == -1)
		{
			endIndex = strLength + 1;
		}

		u64 newStringLength = endIndex - startIndex - numWhitespaceCharacters + 1;
		u64 lastNonWhitespaceIdx{ 0 };
		char* pTrimmedString = new char[newStringLength + 1];

		// Step through the string again, this time copying over all the characters BUT the whitespace characters.
		for (i32 idx = 0, oldIdx = 0; idx < newStringLength; idx++, oldIdx++)
		{
			// Ignore all whitespace tokens on either side of the string.
			while ((pszStr[oldIdx] == ' ' || pszStr[oldIdx] == '\t') && (idx < startIndex || idx > endIndex))
				oldIdx++;

			pTrimmedString[idx] = pszStr[oldIdx];
			lastNonWhitespaceIdx = idx;
		}

		pTrimmedString[lastNonWhitespaceIdx + 1] = 0;
		return pTrimmedString;
	}

	//-------------------------------------------------------------------------
	// Strips all instances of 'c' from pStr.
	// NOTE: This method allocates memory!
	// 
	// Input:	pszStr - The string to be stripped.
	//			c - The character to remove from pszStr.
	// Output:	The stripped string.
	//-------------------------------------------------------------------------
	char* StringStrip(char* pszStr, char c)
	{
		if (!pszStr)
		{
			return nullptr;
		}

		u64 len = StringLength(pszStr);
		u64 numInstances = 0;

		for (i32 idx = 0; idx < len; idx++)
		{
			if (pszStr[idx] == c)
				numInstances++;
		}

		char* newString = new char[(len - numInstances) + 1];
		numInstances = 0;
		for (i32 idx = 0; idx < (len - numInstances); idx++)
		{
			if (pszStr[idx] == c)
			{
				numInstances++;
				continue;
			}

			newString[idx - numInstances] = pszStr[idx];
		}
		newString[(len - numInstances) - 1] = '\0';
		return newString;
	}

	//-------------------------------------------------------------------------
	// Strips all instances of 'c' from pStr.
	// NOTE: This method does not allocate memory.
	// 
	// Input:	pszStr - The string to be stripped.
	//			c - The character to remove from pszStr.
	// Output:	none.
	//-------------------------------------------------------------------------
	void StringStripInPlace(char* pszStr, char c)
	{
		if (!pszStr)
		{
			return;
		}

		u64 len = StringLength(pszStr);
		u64 numInstances = 0;

		for (i32 idx = 0; idx < len; idx++)
		{
			if (pszStr[idx] == c)
			{
				// Strip this character and pull the rest of the string back.
				for (i32 inIdx = idx; inIdx < len; inIdx++)
				{
					pszStr[inIdx] = pszStr[inIdx + 1];
				}
			}
		}
	}

	// NOTE: Our own version of these would be nice.
	//-------------------------------------------------------------------------
	// Purpose: Converts an ascii value to an integer.
	// 
	// Input:	c - The character to convert to an integer.
	// Output:	i32 - The converted character.
	//-------------------------------------------------------------------------
	i32 String_ATOI(const char* c)
	{
		return atoi(c);
	}

	//-------------------------------------------------------------------------
	// Purpose: Converts an ascii value to a floating-point value.
	// 
	// Input:	c - The character to convert to a f32.
	// Output:	f32 - The converted character.
	//-------------------------------------------------------------------------
	f32 String_ATOF(const char* c)
	{
		return static_cast<f32>(atof(c));
	}

	//-------------------------------------------------------------------------
	// Purpose: Converts an ascii string to a Vector3D_OLD.
	// 
	// Input:	c - The character to convert to a f32.
	//			pOutVec - Vector that will hold the converted character.
	// Output:	bool - True if successfully converted, otherwise false.
	//-------------------------------------------------------------------------
	bool String_ToVector3(const char* c, Vector3D* pOutVec)
	{
		i32 numElements = sscanf_s(c, "\"%f %f %f\"", &pOutVec->flElements[0], &pOutVec->flElements[1], &pOutVec->flElements[2]);
		if (numElements == 0)
		{
			// Try without the quotes.
			numElements = sscanf_s(c, "%f %f %f", &pOutVec->flElements[0], &pOutVec->flElements[1], &pOutVec->flElements[2]);
		}

		if (numElements != 3)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

} // namespace Utl
