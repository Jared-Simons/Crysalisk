//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Core Assertion Utilities
//=============================================================================
#pragma once

#include <cassert>
#include <stdexcept>

#define AssertMsg(_msg)		\
	if (_msg)				\
		throw std::runtime_error(_msg); \

