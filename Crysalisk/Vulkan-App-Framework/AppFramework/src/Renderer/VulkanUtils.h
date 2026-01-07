//=============================================================================
// Author: Jared Simons
// Date Created: 6/26/2025
// 
// Purpose: Some Vulkan Renderer utilities.
//=============================================================================
#pragma once

#include <cassert>

#define VK_CHECK(expr)		\
	if (expr != VK_SUCCESS)	\
		assert(0);