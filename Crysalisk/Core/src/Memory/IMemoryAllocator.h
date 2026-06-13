//=============================================================================
// Author: Jared Simons
// Date Created: 6/23/2025
// 
// Purpose: Base memory allocator interface.
//=============================================================================
#pragma once

namespace Utl
{
	enum eAllocatorType
	{
		ALLOCATOR_TYPE_DYNAMIC = 0,
		ALLOCATOR_TYPE_LINEAR,
	};

	class IBaseMemoryAllocator
	{
	public:
		virtual void*	Allocate(u64 numBytes) = 0;
		virtual i32		Free(void* pMemBlock) = 0;
		virtual void	FreeAll() = 0;

		virtual eAllocatorType	GetAllocatorType() const = 0;
	};
} // namespace Utl
