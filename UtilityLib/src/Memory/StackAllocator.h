//=============================================================================
// Author: Jared Simons
// Date Created: 5/19/2024
// 
// Purpose: A Stack-Based Memory Allocator.
//=============================================================================
#pragma once

#include <Platform/Platform.h>
#include <Core/Logging.h>

template<typename T>
class StackAllocator
{
public:
	StackAllocator()
		: m_AllocatorSize{ 0 }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr } {}

	StackAllocator(size_t elementsToAllocate)
		: m_AllocatorSize{ elementsToAllocate * sizeof(T) }
		, m_pData{ nullptr }
		, m_pCurrentAlloc{ nullptr }
	{
		m_pData = (T*)Platform_MemoryAllocate(m_AllocatorSize);
		m_pCurrentAlloc = m_pData;
	}

	~StackAllocator()
	{
		m_AllocatorSize = 0;

		if (m_pData)
			Platform_MemoryFree(m_pData);

		LOG_DEBUG("Stack allocator destroyed");
	}

	T* allocate()
	{
		T* newBlock = m_pCurrentAlloc;
		m_pCurrentAlloc += sizeof(T);

		return newBlock;
	}

	void free(bool allocateNewMemory = true)
	{
		Platform_MemoryFree(m_pData);

		if (allocateNewMemory)
		{
			m_pData = (T*)Platform_MemoryAllocate(m_AllocatorSize);
			m_pCurrentAlloc = m_pData;
		}
		else
		{
			m_pData = nullptr;
			m_pCurrentAlloc = nullptr;
		}
	}

	size_t GetAllocatorSize() const { return m_AllocatorSize; }

private:
	size_t m_AllocatorSize;
	T* m_pData;

	T* m_pCurrentAlloc;
};
