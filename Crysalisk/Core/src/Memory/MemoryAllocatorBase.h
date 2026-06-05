//=============================================================================
// Author: Jared Simons
// Date Created: 6/23/2025
// 
// Purpose: Base memory allocator.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Memory/IMemoryAllocator.h"

namespace Utl
{
	class CORE_API CBaseMemoryAllocator : public IBaseMemoryAllocator
	{
	public:
		virtual void*	Allocate(size_t numBytes) override;
		virtual void	Free(void* pMemBlock) override;
		virtual void	FreeAll() override;

		// Factory method to return a default memory allocator.
		CBaseMemoryAllocator	CreateDefaultMemoryAllocator()
		{
			CBaseMemoryAllocator newAllocator{};

			return newAllocator;
		}
	};
} // namespace Utl