//=============================================================================
// Author: Jared Simons
// Date Created: 5/19/2024
// 
// Purpose: A Linear Memory Allocator.
// Can allocate in chunks, but freed all at once.
//=============================================================================
#pragma once

#include <Core/Logging.h>
#include <Systems/IMemory.h>

class LinearAllocator
{
public:
	LinearAllocator() = delete;

	LinearAllocator(size_t bytesToAllocate)
		: m_AllocatorSize{ bytesToAllocate }
		, m_BytesAllocated{ 0 }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr }
	{
		m_pData = gpMemorySystem->MemAllocate(m_AllocatorSize, eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);
		m_pCurrentAlloc = m_pData;
	}

	~LinearAllocator()
	{
		if (m_BytesAllocated != 0)
			free();

		if (m_pData)
			gpMemorySystem->MemFree(m_pData, m_AllocatorSize, eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);

		m_AllocatorSize = 0;

		//LOG_DEBUG("Stack allocator destroyed");
	}

	void* allocate(size_t size)
	{
		if (m_BytesAllocated == m_AllocatorSize)
		{
			LOG_ERROR("StackAllocator->Allocate() - Cannot allocate anymore memory, returning nullptr.");
			return nullptr;
		}

		void* newBlock = (char*)m_pData + m_BytesAllocated;
		m_BytesAllocated += size;

		return newBlock;
	}

	void free(bool allocateNewMemory = true)
	{
		gpMemorySystem->MemFree(m_pData, sizeof(m_pData), eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);
		m_BytesAllocated = 0;

		if (allocateNewMemory)
		{
			m_pData = gpMemorySystem->MemAllocate(m_AllocatorSize, eMemAllocCategories::MEM_CATEGORY_LINEAR_ALLOC);
			m_pCurrentAlloc = m_pData;
		}
		else
		{
			m_pData = nullptr;
			m_pCurrentAlloc = nullptr;
		}
	}

	size_t GetAllocatorSize() const { return m_AllocatorSize; }
	size_t GetBytesAllocated() const { return m_BytesAllocated; }

	void* GetMemory() const { return m_pData; }

private:
	size_t m_AllocatorSize;
	size_t m_BytesAllocated;
	void* m_pData;

	void* m_pCurrentAlloc;
};
