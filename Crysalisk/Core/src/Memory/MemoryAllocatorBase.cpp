//=============================================================================
// Author: Jared Simons
// Date Created: 6/23/2025
// 
// Purpose: Base memory allocator.
//=============================================================================
#include "MemoryAllocatorBase.h"

#include "Memory/MemoryManager.h"

namespace Utl
{
	void* CBaseMemoryAllocator::Allocate(size_t numBytes)
	{
		return g_pMemoryManager->Allocate(numBytes);
	}

	void CBaseMemoryAllocator::Free(void* pMemBlock)
	{
		g_pMemoryManager->Free(pMemBlock);
	}

	void CBaseMemoryAllocator::FreeAll()
	{

	}
} // namespace Utl