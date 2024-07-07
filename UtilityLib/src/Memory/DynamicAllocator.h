//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2024
// 
// Purpose: A Dynamic Memory Allocator.
//=============================================================================
#pragma once

#include "../Containers/CFreeList.h"

// TODO: TEMP
#include <malloc.h>
// TODO: END TEMP


struct MemoryBlockMetadata_t
{
	// The size of the memory block
	size_t size;
	// The offset into the memory block
	size_t offset;
	// The memory block's alignment
	size_t alignment;
};

const size_t ALLOC_BLOCK_HEADER_SIZE{ sizeof(MemoryBlockMetadata_t) };

class DynamicAllocator
{
public:
	DynamicAllocator(size_t allocatorSize)
		: m_TotalSize{ allocatorSize }
		, m_pMemory{ nullptr }
		, m_FreeList{ allocatorSize }
	{
		m_pMemory = malloc(allocatorSize);
	}

	~DynamicAllocator()
	{
		if (m_pMemory)
			free(m_pMemory);

		m_pMemory = nullptr;
	}

	void* Allocate(size_t size)
	{
		// Size of allocation plus the size required to store the header.
		size_t requiredSize = size + ALLOC_BLOCK_HEADER_SIZE;

		// Allocate the memory
		size_t offset = 0;
		void* memoryBlock = nullptr;

		if (m_FreeList.Allocate(requiredSize, &offset))
		{
			// The start of the block will store the header data 
			// Then will move forward by ALLOC_BLOCK_HEADER_SIZE bytes for the data section (which will be returned).
			MemoryBlockMetadata_t* metaData = (MemoryBlockMetadata_t*)m_pMemory + offset;

			// TODO: Memory alignment
			metaData->alignment = 1;
			metaData->offset = offset;
			metaData->size = size;

			// Push the pointer forward.
			memoryBlock = (uintptr_t*)m_pMemory + offset + ALLOC_BLOCK_HEADER_SIZE;

			return memoryBlock;
		}
		else
		{
			// Could not allocate memory block
			LOG_ERROR("DynamicAllocator->Allocate(): Failed to allocate memory block. Returning nullptr.");
			return nullptr;
		}
	}

	void Free(void* pMemBlock)
	{
		if (!pMemBlock)
		{
			LOG_ERROR("DynamicAllocator->Free(): Requires valid pointer to memory block. Nothing was done.");
			return;
		}

		// Retrieve the data from the block's header.
		void* blockHeader = (uintptr_t*)pMemBlock - ALLOC_BLOCK_HEADER_SIZE;
		MemoryBlockMetadata_t* metaData = (MemoryBlockMetadata_t*)blockHeader;

		m_FreeList.Free(metaData->size + ALLOC_BLOCK_HEADER_SIZE, metaData->offset);
	}

	size_t GetFreeSpace() const { return m_FreeList.GetFreeSpace(); }

private:
	size_t		m_TotalSize;
	void*		m_pMemory;
	CFreeList	m_FreeList;
};
