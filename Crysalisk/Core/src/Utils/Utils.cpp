//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: Some generic utility functions.
//=============================================================================
#pragma once

#include "Utils.h"

#include "Math/MathLib.h"
#include "Strings/String_Utils.h"

// Hashing function borrowed from "https://opendsa-server.cs.vt.edu/ODSA/Books/CS3/html/HashFuncExamp.html"
int SimpleHash_StringToInt(const char* pInput, u64 numElements)
{
	int sum = 0;
	int mul = 1;

	u64 len = Utl::StringLength(pInput);

	for (int idx = 0; idx < len; idx++)
	{
		mul = (idx % 4 == 0) ? 1 : mul * 256;
		sum += pInput[idx] * mul;
	}

	return static_cast<int>((u64)M_ABS(sum) % numElements);
}

u16 PackF32ToUint16(const f32 flUnitFloat)
{
	const u32 numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

	// Scale the unit value to fit within this range.
	f32 flScaledFloat = flUnitFloat * (numIntervals - 1U);

	// Round and truncate to an int.
	u16 nRounded = (u16)(flScaledFloat + 0.5f);

	// Make sure we don't go outside of our valid range.
	if (nRounded > numIntervals - 1)
		nRounded = numIntervals - 1U;

	return nRounded;
}

f32 UnpackF32FromUint16(const u16 inputData)
{
	const u32 numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

	// Scale the data and store in a f32.
	f32 flUnitFloat = ((f32)inputData / numIntervals);

	return flUnitFloat;
}
