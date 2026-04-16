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
int SimpleHash_StringToInt(const char* pInput, size_t numElements)
{
	int sum = 0;
	int mul = 1;

	size_t len = Utl::StringLength(pInput);

	for (int idx = 0; idx < len; idx++)
	{
		mul = (idx % 4 == 0) ? 1 : mul * 256;
		sum += pInput[idx] * mul;
	}

	return static_cast<int>((size_t)M_ABS(sum) % numElements);
}

uint16_t PackF32ToUint16(const float flUnitFloat)
{
	const unsigned int numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

	// Scale the unit value to fit within this range.
	float flScaledFloat = flUnitFloat * (numIntervals - 1U);

	// Round and truncate to an int.
	uint16_t nRounded = (uint16_t)(flScaledFloat + 0.5f);

	// Make sure we don't go outside of our valid range.
	if (nRounded > numIntervals - 1)
		nRounded = numIntervals - 1U;

	return nRounded;
}

float UnpackF32FromUint16(const uint16_t inputData)
{
	const unsigned int numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

	// Scale the data and store in a float.
	float flUnitFloat = ((float)inputData / numIntervals);

	return flUnitFloat;
}
