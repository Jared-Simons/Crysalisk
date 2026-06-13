//=============================================================================
// Author: Jared Simons
// Date Created: 4/4/2025
// 
// Purpose: Dynamic memory allocator, managed with a freelist.
//=============================================================================
#pragma once

#include "Containers/CFreeList.h"
#include "Memory/IMemoryAllocator.h"

// Stores metadata about heap allocations made by freelist allocators.
struct AllocHeader_t
{
	u64	Size{ 0 };
	u64	Offset{ 0 };
};

const int kAllocHeaderSize = sizeof(AllocHeader_t);

namespace Utl
{
	// A dynamic memory allocator that tracks memory state with a freelist.
	class DynamicAllocator : public IBaseMemoryAllocator
	{
	public:
		// Constructor.
		DynamicAllocator();
		~DynamicAllocator();

		// Initialize internal state for the allocator. This MUST be called before this allocator can do any allocations.
		// NOTE: 'pMemoryBlock' is to be allocated and freed by client code.
		void InitAllocatorInternals(u64 sizeInBytes, void* pMemoryBlock, bool bStoreFreelistInMemoryBlock);

		// Called after 'InitAllocatorInternals()', this sets up the internal freelist.
		void InitFreelist(void* pMemoryBlock);

		// Get the size in bytes required for the internal freelist.
		inline u64 GetInternalFreelistMemoryRequirement() const { return m_FreeList.GetInternalMemoryRequirement(); }

		// Allocates a block of memory.
		void* Allocate(u64 size);

		// Free a block of memory.
		int Free(void* pMemoryBlock);

		virtual void FreeAll() override { CASSERTMSG(0, "DynamicAllocator currently does not support FreeAll(), use Free instead."); }

		virtual eAllocatorType	GetAllocatorType() const override { return ALLOCATOR_TYPE_DYNAMIC; }

	private:
		// The number of bytes that this allocator is tracking/can allocate from.
		u64		m_SizeInBytes;
		// A pointer to the base of the memory block this allocator is allocating from.
		void*		m_pMemoryBlock;
		// The freelist this allocator is using to track allocations.
		CFreeList	m_FreeList;
	};
}