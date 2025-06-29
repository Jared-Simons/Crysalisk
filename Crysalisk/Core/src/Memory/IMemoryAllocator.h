//=============================================================================
// Author: Jared Simons
// Date Created: 6/23/2025
// 
// Purpose: Base memory allocator interface.
//=============================================================================
#pragma once

namespace Utl
{
	class IBaseMemoryAllocator
	{
	public:
		virtual void*	Allocate(size_t numBytes) = 0;
		virtual void	Free(void* pMemBlock) = 0;
	};
} // namespace Utl