//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Core Assertion Utilities
//=============================================================================
#pragma once

#include <cassert>

#include <Core/Logging.h>

#define CAssert(expr)																						\
	if (!(expr))																							\
		{																									\
			LOG_ERROR("Assertion Failure! File: %s, Line: %d", __FILE__, __LINE__);							\
			__debugbreak();																					\
		}

#define CASSERTMSG(expr, msg)																				\
	if (!(expr))																							\
		{																									\
			if (msg)																						\
			{																								\
				LOG_ERROR("Assertion failure!: %s [File: '%s', Line: %d]", msg, __FILE__, __LINE__);		\
			}																								\
		__debugbreak();																						\
	}

