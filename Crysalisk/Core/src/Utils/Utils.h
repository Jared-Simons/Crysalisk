//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: Some generic utility functions.
//=============================================================================
#pragma once

#include "Defines.h"
#include "Math/MathLib.h"
#include "Strings/String_Utils.h"

DLL_EXPORT int SimpleHash_StringToInt(const char* pStr, size_t numElements);

template<typename typeIn, typename typeOut>
typeOut SimpleHash(typeIn pInput, size_t numElements)
{
	typeOut sum = 0;
	int mul = 1;

	size_t len = StringLength(pStr);

	for (int idx = 0; idx < len; idx++)
	{
		mul = (idx % 4 == 0) ? 1 : mul * 256;
		sum += pStr[idx] * mul;
	}

	return static_cast<typeOut>((size_t)M_ABS(sum) % numElements);
}

// Non-templated string version of the above. To keep things simple until we need a more generic hashing structure.
int SimpleHash_StringToInt(const char* pInput, size_t numElements);

// Packing utilities.
uint16_t PackF32ToUint16(const float flUnitFloat);
float UnpackF32FromUint16(const uint16_t inputData);
