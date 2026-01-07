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