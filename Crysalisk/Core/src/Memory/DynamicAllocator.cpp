//=============================================================================
// Author: Jared Simons
// Date Created: 4/4/2025
// 
// Purpose: Dynamic memory allocator, managed with a freelist.
//=============================================================================
#include "DynamicAllocator.h"

namespace Utl
{
	//-------------------------------------------------------------------------
	// Purpose: A constructor. Constructs things.
	//-------------------------------------------------------------------------
	DynamicAllocator::DynamicAllocator()
	{
	}

	//-------------------------------------------------------------------------
	// Purpose: Destructor.
	//-------------------------------------------------------------------------
	DynamicAllocator::~DynamicAllocator()
	{
	}

	//-------------------------------------------------------------------------
	// Purpose: Initialize the allocators internals. Must be called prior to any allocations.
	// 
	// Input:  sizeInBytes - The size in bytes that this allocator will track 
	//				(this includes the space reserved for storing the freelist).
	//		   pMemoryBlock - This is a memory block of size 'sizeInBytes' that will be allocated from.
	//				The lifetime of this memory needs to be handled externally.
	//		   bStoreFreelistInMemoryBlock - This should ALWAYS be true, as the alternative is not yet implemented...
	// Output: none.
	//-------------------------------------------------------------------------
	void DynamicAllocator::InitAllocatorInternals(u64 sizeInBytes, void* pMemoryBlock, bool bStoreFreelistInMemoryBlock = true)
	{
		CAssert(sizeInBytes > 0 && pMemoryBlock != nullptr);
		CASSERTMSG(bStoreFreelistInMemoryBlock == true, "DynamicAllocators currently need to store the freelist in the provided memory block!")
		m_SizeInBytes = sizeInBytes;
		m_pMemoryBlock = pMemoryBlock;
		m_FreeList.SetNumBytesToTrack(sizeInBytes);

		if (bStoreFreelistInMemoryBlock)
		{
			// Initialize the freelist now.
			m_FreeList.InitFreelist(pMemoryBlock);
			u64 freelistArraySize = m_FreeList.GetInternalMemoryRequirement();
			m_pMemoryBlock = reinterpret_cast<u8*>(m_pMemoryBlock) + freelistArraySize;
			m_FreeList.SetNumBytesToTrack(sizeInBytes - freelistArraySize);
		}
	}

	//-------------------------------------------------------------------------
	// Purpose: Initialize the internal freelist. Must be called prior to any allocations.
	// 
	// Input:  pMemoryBlock - The block of memory that the freelist will be 'placed' into.
	//				CANNOT be nullptr!
	// Output: none.
	//-------------------------------------------------------------------------
	void DynamicAllocator::InitFreelist(void* pMemoryBlock)
	{
		CAssert(pMemoryBlock != nullptr);
		m_FreeList.InitFreelist(pMemoryBlock);
	}

	//-------------------------------------------------------------------------
	// Purpose: Allocates a block of memory from the allocator.
	// 
	// Input:  size - The size in bytes of the allocation to make.
	//				Must be > 0!
	// Output: A pointer to the allocated memory.
	//-------------------------------------------------------------------------
	void* DynamicAllocator::Allocate(u64 size)
	{
		CAssert(size > 0);
		void* pNewBlock = nullptr;

		u64 offset = m_FreeList.Allocate(size + kAllocHeaderSize);
		AllocHeader_t* newHeader = reinterpret_cast<AllocHeader_t*>(reinterpret_cast<u8*>(m_pMemoryBlock) + offset);
		newHeader->Offset = offset;
		newHeader->Size = size;
		pNewBlock = reinterpret_cast<u8*>(newHeader) + kAllocHeaderSize;

		return pNewBlock;
	}

	//-------------------------------------------------------------------------
	// Purpose: Frees a block of memory owned by the allocator.
	// 
	// Input:  pMemoryBlock - A pointer to the base of the block of memory
	//				allocated by Allocate(). Must not be nullptr!
	// Output: none.
	//-------------------------------------------------------------------------
	i32 DynamicAllocator::Free(void* pMemoryBlock)
	{
		CAssert(pMemoryBlock != nullptr);

		AllocHeader_t* pHeader = reinterpret_cast<AllocHeader_t*>(reinterpret_cast<u8*>(pMemoryBlock) - kAllocHeaderSize);

		// Mark the memory as being free in the freelist.
		m_FreeList.Free(pHeader->Size + kAllocHeaderSize, pHeader->Offset);

		return (i32)pHeader->Size;
	}
}