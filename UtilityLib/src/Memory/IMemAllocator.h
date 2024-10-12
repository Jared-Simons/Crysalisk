//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2024
// 
// Purpose: A generic memory allocator interface.
//=============================================================================
#pragma once

#include <Defines.h>

class IMemAllocator
{
public:
	virtual void* Allocate(u64 sizeInBytes, u64 alignment = 2) = 0;
	virtual void Free(void* pMemBlock) = 0;
};