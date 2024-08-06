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
		, m_pData{ nullptr } {}

	CDeque(size_t capacity)
		: m_Count{ 0 }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_pData{ nullptr }
	{
		m_pData = gpMemorySystem->MemAllocate(m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);
	}

	virtual ~CDeque()
	{
		if (m_pData)
			gpMemorySystem->MemFree(m_pData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);

		m_pData = nullptr;
	}

	CDeque& operator=(const CDeque& other)
	{
		// TODO: Implement
	}

	T& operator[] (int index)
	{
		return m_pData[index];
	}

	bool Empty() const { return m_Capacity == 0 && m_Count == 0 && m_pData == nullptr; }

	T& Front() const { return m_pData[0]; }

	// Erases all the elements in the queue and zeros out the memory.
	// After calling this method, the size of the deque returns to zero.
	void Clear()
	{
		gpMemorySystem->MemSet(m_pData, 0, m_Stride * m_Capacity);
		m_Count = 0;
		m_Capacity = 0;
	}

	void Resize(size_t newCapacity)
	{
		if (newCapacity == 0)
		{
			// Just a safety check
			// If the capacity is 0, the buffer will never resize.
			newCapacity = 1 * m_ResizeFactor; // Allocate room for 2 elements by default
		}

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
			T* pNewMemory = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);
			memset(pNewMemory, 0, m_Stride * newCapacity);

			// Copy the original data over (as much as can be held anyways)
			memcpy((void*)pNewMemory, (void*)m_pData, m_Stride * newCapacity);

			gpMemorySystem->MemFree(m_pData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			// Assign the new pointer
			m_pData = pNewMemory;
			m_Count = newCapacity; // count should be equal to capacity here since we are downsizing
			m_Capacity = newCapacity;
		}
		else
		{
			// Resizing to a larger capacity,
			// Just copy the elements over and insert extra
			T* pNewMemory = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);
			memset(pNewMemory, 0, m_Stride * newCapacity);

			if (m_pData)
			{
				memcpy((void*)pNewMemory, (void*)m_pData, m_Stride * m_Count);
				memset(m_pData, 0, m_Stride * m_Count);
			}

			gpMemorySystem->MemFree(m_pData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			// Assign the new pointer
			m_pData = pNewMemory;
			m_Capacity = newCapacity;
		}
	}

	T& Insert(size_t pos, const T& value)
	{
		if (pos > m_Capacity)
		{
			// The requested index is out of range of our allocated memory. Boot out.
			// Return a copy to the base of our data block, just to be safe.
			LOG_ERROR("CDeque::Insert() -> Requested memory index is out of range of the queue's memory buffer. Nothing was done.");
			return *m_Data;
		}

		if (m_Count + 1 > m_Capacity)
		{
			// We need to resize to make space for more data.
			Resize(m_Capacity * m_ResizeFactor);

			// TODO: Add an assert here
			if (pos > m_Capacity)
				LOG_ERROR("CDeque::Resize() -> Did not acquire enough new memory.");

			if (m_Count < 1)
			{
				// We just resized an empty queue to hold some amount of data.
				// No need to copy
				m_pData[pos] = value;
				m_Count++;
				return m_pData[pos];
			}

			// We don't need to resize ourselves and our index is in range
			// Insert the value before the index (pos 3 means that every value past index 3 is pushed back by 1 value)
			size_t numElementsToMove = m_Count - pos;

			if (numElementsToMove == 0)
				numElementsToMove = 1;

			// Allocate a temporary buffer and copy the elements into it
			void* tempBuffer = gpMemorySystem->MemAllocate(m_Stride * numElementsToMove, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			memcpy((void*)tempBuffer, (void*)((T*)m_pData + pos), m_Stride * numElementsToMove); // NOTE: Need to do weird casting here to do the pointer arithmetic
			memset((void*)((T*)m_pData + pos), 0, (m_Stride * numElementsToMove) + m_Stride); // Zero out all of the memory from the insert index to the end of the buffer

			// Insert the value at the requested index.
			m_pData[pos] = value;
			m_Count++;

			// Copy the old data back over and free the buffer
			memcpy((void*)((T*)m_pData + pos + 1), tempBuffer, m_Stride * numElementsToMove);
			gpMemorySystem->MemFree(tempBuffer, m_Stride * numElementsToMove, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			return m_pData[pos];
		}
		else
		{
			// We don't need to resize ourselves and our index is in range
			// Insert the value before the index (pos 3 means that every value past index 3 is pushed back by 1 value)
			size_t numElementsToMove = m_Count - pos;

			if (m_Count < pos)
			{
				// We don't need to move any elements around, just insert the element at the index
				m_pData[pos] = value;
				m_Count++;
				return m_pData[pos];
			}

			if (pos == m_Capacity)
			{
				// We are inserting at the end of the buffer, no copy is necessary
				m_pData[pos] = value;
				m_Count++;
				return m_pData[pos];
			}

			// Allocate a temporary buffer and copy the elements into it
			void* tempBuffer = gpMemorySystem->MemAllocate(m_Stride * numElementsToMove, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			memcpy((void*)tempBuffer, (void*)((T*)m_pData + pos), m_Stride * numElementsToMove); // NOTE: Need to do weird casting here to do the pointer arithmetic
			memset((void*)((T*)m_pData + pos), 0, (m_Stride * numElementsToMove) + m_Stride); // Zero out all of the memory from the insert index to the end of the buffer

			// Insert the new element
			m_pData[pos] = value;
			m_Count++;

			// Copy the old data back over and free the buffer
			memcpy((void*)((T*)m_pData + pos + 1), tempBuffer, m_Stride * numElementsToMove);
			gpMemorySystem->MemFree(tempBuffer, m_Stride * numElementsToMove, eMemAllocCategories::MEM_CATEGORY_QUEUE);

			return m_pData[pos];
		}
	}

	void PushBack(const T& value)
	{
		if (m_Count + 1 > m_Capacity)
		{
			// Need to resize the buffer to make more room
			Resize(m_Capacity * m_ResizeFactor);
		}

		m_pData[m_Count] = value;
		m_Count++;
	}

	void PushFront(const T& value)
	{
		if (m_Count + 1 > m_Capacity)
		{
			// Need to resize the buffer to make more room
			Resize(m_Capacity * m_ResizeFactor);
		}

		// Grab all existing elements and push them over one element space.
		void* tempBuffer = gpMemorySystem->MemAllocate(m_Count * m_Stride, eMemAllocCategories::MEM_CATEGORY_QUEUE);
		memcpy(tempBuffer, m_pData, m_Count * m_Stride);
		memset(m_pData, 0, m_Capacity * m_Stride);

		memcpy((void*)((T*)m_pData + 1), tempBuffer, m_Count * m_Stride);
		gpMemorySystem->MemFree(tempBuffer, m_Count * m_Stride, eMemAllocCategories::MEM_CATEGORY_QUEUE);

		m_Count++;
		m_pData[0] = value;
	}

	void PopFront()
	{
		memset((void*)((T*)m_pData), 0, m_Stride);

		void* tempBuffer = gpMemorySystem->MemAllocate((m_Count - 1) * m_Stride, eMemAllocCategories::MEM_CATEGORY_QUEUE);
		memcpy(tempBuffer, (void*)(T*)(m_pData + 1), (m_Count - 1) * m_Stride);
		memset(m_pData, 0, m_Capacity * m_Stride);

		memcpy((void*)((T*)m_pData), tempBuffer, (m_Count - 1) * m_Stride);

		m_Count--;
	}

	size_t			count() const { return m_Count; }
	size_t			capacity() const { return m_Capacity; }
	size_t			stride() const { return m_Stride; }

	unsigned int	GetResizeFactor() const { return m_ResizeFactor; }
	void			SetResizeFactor(size_t newResizeFactor) { m_ResizeFactor = newResizeFactor; }

	T*				data() { return m_pData; }

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
	T* m_pData;
};
