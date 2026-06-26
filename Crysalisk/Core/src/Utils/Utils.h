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

DLL_EXPORT i32 SimpleHash_StringToInt(const char* pStr, u64 numElements);

template<typename typeIn, typename typeOut>
typeOut SimpleHash(typeIn pInput, u64 numElements)
{
	typeOut sum = 0;
	i32 mul = 1;

	u64 len = StringLength(pStr);

	for (i32 idx = 0; idx < len; idx++)
	{
		mul = (idx % 4 == 0) ? 1 : mul * 256;
		sum += pStr[idx] * mul;
	}

	return static_cast<typeOut>((u64)M_ABS(sum) % numElements);
}

DLL_EXPORT inline bool FloatCompare(f32 a, f32 b, f32 tolerance = 0.0000001f);

// Non-templated string version of the above. To keep things simple until we need a more generic hashing structure.
i32 SimpleHash_StringToInt(const char* pInput, u64 numElements);

// Packing utilities.
u16 PackF32ToUint16(const f32 flUnitFloat);
f32 UnpackF32FromUint16(const u16 inputData);
