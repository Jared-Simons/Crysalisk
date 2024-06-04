//=============================================================================
// Author: Jared Simons
// Date Created: 5/19/2024
// 
// Purpose: A Stack-Based Memory Allocator.
//=============================================================================
#pragma once

#include <Core/Logging.h>
#include <Systems/IMemory.h>

template<typename T>
class StackAllocator
{
public:
	StackAllocator()
		: m_AllocatorSize{ 0 }
		, m_BytesAllocated{ 0 }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr } {}

	StackAllocator(size_t elementsToAllocate)
		: m_AllocatorSize{ elementsToAllocate * sizeof(T) }
		, m_BytesAllocated{ 0 }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr }
	{
		m_pData = (T*)gpMemorySystem->MemAllocate(m_AllocatorSize);
		m_pCurrentAlloc = m_pData;
	}

	~StackAllocator()
	{
		m_AllocatorSize = 0;

		if (m_BytesAllocated != 0)
			free();

		if (m_pData)
			gpMemorySystem->MemFree(m_pData);

		//LOG_DEBUG("Stack allocator destroyed");
	}

	T* allocate()
	{
		if (m_BytesAllocated == m_AllocatorSize)
		{
			LOG_ERROR("StackAllocator->Allocate() - Cannot allocate anymore memory, returning nullptr.");
			return nullptr;
		}

		T* newBlock = m_pCurrentAlloc;
		m_BytesAllocated += sizeof(T);
		m_pCurrentAlloc += 1;

		return newBlock;
	}

	void free(bool allocateNewMemory = true)
	{
		gpMemorySystem->MemFree(m_pData);
		m_BytesAllocated = 0;

		if (allocateNewMemory)
		{
			m_pData = (T*)gpMemorySystem->MemAllocate(m_AllocatorSize);
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

	T* GetMemory() const { return m_pData; }

private:
	size_t m_AllocatorSize;
	size_t m_BytesAllocated;
	T* m_pData;

	T* m_pCurrentAlloc;
};
