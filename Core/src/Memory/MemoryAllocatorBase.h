//=============================================================================
// Author: Jared Simons
// Date Created: 6/23/2025
// 
// Purpose: Base memory allocator.
//=============================================================================
#pragma once

#include <Defines.h>

namespace Utl
{
	class CORE_API CBaseMemoryAllocator
	{
	public:
		void*	Allocate(size_t numBytes);
		void	Free(void* pMemBlock);
	};
} // namespace Utl