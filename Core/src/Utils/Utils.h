//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: Some generic utility functions.
//=============================================================================
#pragma once

#include "Defines.h"

DLL_EXPORT int SimpleHashToInt(const char* pStr, size_t numElements);

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