//=============================================================================
// Author: Jared Simons
// Date Created: 5/19/2024
// 
// Purpose: A Linear Memory Allocator.
// Can allocate in chunks, but freed all at once.
//=============================================================================
#pragma once

#include <Defines.h>
#include "IMemAllocator.h"

#include <Core/Logging.h>
#include <Core/Memory.h>

#include <cstring>

class LinearAllocator : public IMemAllocator
{
public:
	LinearAllocator() = delete;

	LinearAllocator(u64 bytesToAllocate)
		: m_AllocatorSize{ bytesToAllocate }
		, m_BytesAllocated{ 0 }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr }
	{
		m_pData = Memory::Allocate(m_AllocatorSize, eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);
		m_pCurrentAlloc = m_pData;
	}

	~LinearAllocator()
	{
		if (m_BytesAllocated != 0)
			Free();

		if (m_pData)
			Memory::Free(m_pData, m_AllocatorSize, eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);

		m_AllocatorSize = 0;

		//LOG_DEBUG("Stack allocator destroyed");
	}

	void* Allocate(u64 size, u64 alignment = 2) override
	{
		if (m_AllocatorSize <= size + m_BytesAllocated)
		{
			LOG_ERROR("StackAllocator->Allocate() - Cannot allocate anymore memory, returning nullptr.");
			return nullptr;
		}

		if (m_BytesAllocated == m_AllocatorSize)
		{
			LOG_ERROR("StackAllocator->Allocate() - Cannot allocate anymore memory, returning nullptr.");
			return nullptr;
		}

		void* newBlock = (char*)m_pData + m_BytesAllocated;
		m_BytesAllocated += size;

		memset(newBlock, 0, size);

		return newBlock;
	}

	void Free(bool allocateNewMemory = true)
	{
		// Just zero out our memory.
		if (m_pData)
		{
			Memory::MemSet(m_pData, 0, m_AllocatorSize);
		}
		m_BytesAllocated = 0;
	}

	// We can't linearly free memory.
	void Free(void* pMemBlock) override {};

	u64 GetAllocatorSize() const { return m_AllocatorSize; }
	u64 GetBytesAllocated() const { return m_BytesAllocated; }

	void* GetMemory() const { return m_pData; }

private:
	u64 m_AllocatorSize;
	u64 m_BytesAllocated;
	void* m_pData;

	void* m_pCurrentAlloc;
};
