//=============================================================================
// Author: Jared Simons
// Date Created: 4/4/2025
// 
// Purpose: Dynamic memory allocator, managed with a freelist.
//=============================================================================
#pragma once

#include "Containers/CFreeList.h"

struct AllocHeader_t
{
	size_t	Size{ 0 };
	size_t	Offset{ 0 };
};

const int kAllocHeaderSize = sizeof(AllocHeader_t);

namespace Utl
{
	class DynamicAllocator
	{
	public:
		DynamicAllocator();

		void InitAllocatorInternals(size_t pSizeInBytes, void* pMemoryBlock, bool bStoreFreelistInMemoryBlock);

		void InitFreelist(void* pMemoryBlock);

		inline size_t GetInternalFreelistMemoryRequirement() const { return m_FreeList.GetInternalMemoryRequirement(); }

		void* Allocate(size_t size);

		int Free(void* pMemoryBlock);

	private:
		size_t		m_SizeInBytes;
		void*		m_pMemoryBlock;
		CFreeList	m_FreeList;
	};
}