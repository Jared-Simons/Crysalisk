//=============================================================================
// Author: Jared Simons
// Date Created: 7/31/2024
// 
// Purpose: A Custom Deque Data Structure.
//=============================================================================
#pragma once

#include <Core/Logging.h>
#include <Systems/IMemory.h>

template <typename T>
class CDeque
{
public:
	CDeque()
		: m_Count{ 0 }
		, m_Capacity{ 0 }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_Data{ nullptr } {}

	CDeque(size_t capacity)
		: m_Count{ 0 }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_Data{ nullptr }
	{
		m_Data = gpMemorySystem->MemAllocate(capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);
	}

	virtual ~CDeque()
	{
		gpMemorySystem->MemFree(m_Data, sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);
	}

	CDeque& operator=(const CDeque& other)
	{
		// TODO: Implement
	}

	// Erases all the elements in the deque and zeros out the memory.
	// After calling this method, the size of the deque returns to zero.
	void Clear()
	{
		gpMemorySystem->MemSet(m_Data, 0, sizeof(T) * m_Capacity);
		m_Count = 0;
		m_Capacity = 0;
	}

	void Resize(size_t newCapacity)
	{
		if (m_Capacity == newCapacity)
		{
			// Don't do anything here.
			return;
		}

		if (m_Capacity > newCapacity)
		{
			// We are resizing to a smaller capacity,
			// Take the first number of elements that will fit in the new queue
			
			// Allocate the new memory block
			T* pNewMemory = (T*)gpMemorySystem->MemAllocate(sizeof(T) * newCapacity);

			// Copy the original data over (as much as can be held anyways)
			Platform_MemoryCopy(pNewMemory, m_Data, sizeof(T) * newCapacity);

			Platform_MemoryFree(m_Data);

			// Assign the new pointer
			m_Data = pNewMemory;
			m_Count = newCapacity; // count should be equal to capacity here since we are downsizing
			m_Capacity = newCapacity;
		}
		else
		{
			// Resizing to a larger capacity,
			// Just copy the elements over and insert extra
			T* pNewMemory = (T*)gpMemorySystem->MemAllocate(sizeof(T) * newCapacity);

			Platform_MemoryCopy(pNewMemory, m_Data, sizeof(T) * m_Count);

			Platform_MemoryFree(m_Data);

			// Assign the new pointer
			m_Data = pNewMemory;
			m_Capacity = newCapacity;
		}
	}

	T& Insert(size_t pos, const T& value)
	{
		if (pos >= m_Capacity)
		{
			// The requested index is out of range of our allocated memory. Boot out.
			LOG_ERROR("CDeque::Insert() -> Requested memory index is out of range of the queue's memory buffer. Nothing was done.");
			return 0;
		}

		if (m_Count + 1 > m_Capacity)
		{
			// We need to resize to make space for more data.
			Resize(m_Capacity * m_ResizeFactor);

			// TODO: Add an assert here
			if (pos > m_Capacity)
				LOG_ERROR("CDeque::Resize() -> Did not acquire enough new memory.");
		}
	}

private:
	/* The current number of elements in the queue (not necessarily the capacity) */
	size_t m_Count;

	/* The maximum number of elements that can be held in the queue. (Can be resized, of course) */
	size_t m_Capacity;

	/* The stride of the elements (the length in bytes between each element) */
	size_t m_Stride;

	/* The multiplier to be used when the queue resizes to make more room. */
	unsigned int m_ResizeFactor;

	/* The data included in the queue, can be accessed with the [] operators */
	T* m_Data;
};
