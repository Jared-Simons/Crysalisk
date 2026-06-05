//=============================================================================
// Author: Jared Simons
// Date Created: 6/4/2026
// 
// Purpose: A linear allocator.
//=============================================================================
#include "LinearAllocator.h"

#include <Core/Assertions.h>

namespace Utl
{
	bool LinearAllocator::Initialize(u64 iAllocatorSize)
	{
		if (iAllocatorSize < 1)
		{
			CAssert(0);
			LOG_ERROR("LinearAllocator::Initialize() -> size was < 1. Allocator could not be initialized.");
			return false;
		}

		// Allocate a block of memory for us to use.
		m_pMemory = new u8[iAllocatorSize];

		if (m_pMemory == nullptr)
		{
			LOG_ERROR("Failed to allocate memory for LinearAllocator usage. Initialization failed.");
			return false;
		}

		m_iTotalAllocatorSize = iAllocatorSize;
		m_iMemoryHeadPtr = 0;
		m_bAllocatorOwnsMemory = true;
		return true;
	}

	void LinearAllocator::Destroy()
	{
		if (m_pMemory != nullptr && m_bAllocatorOwnsMemory)
		{
			delete[] m_pMemory;
			m_pMemory = nullptr;
		}
		Platform::MemorySet(this, 0, sizeof(LinearAllocator));
	}

	void* LinearAllocator::Allocate(u64 numBytes)
	{
		CAssert(numBytes > 0);
		if (numBytes < 1)
		{
			LOG_WARN("LinearAllocator::Allocate called with numBytes < 1. Returning nullptr.");
			return nullptr;
		}

		void* pMemToReturn = (u8*)m_pMemory + m_iMemoryHeadPtr;
		m_iMemoryHeadPtr += numBytes;

		return pMemToReturn;
	}
	
	int LinearAllocator::Free(void* pMemBlock)
	{
		CASSERTMSG(0, "LinearAllocator does not support Free(), call FreeAll()!");
		return 0;
	}

	void LinearAllocator::FreeAll()
	{
		CAssert(m_pMemory != nullptr);
		// Zero out the whole block
		Platform::MemorySet(m_pMemory, 0, m_iTotalAllocatorSize);
		m_iMemoryHeadPtr = 0;
	}
} // namespace Utl