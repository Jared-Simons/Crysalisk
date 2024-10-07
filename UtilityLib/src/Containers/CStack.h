//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: A Custom Stack Data Structure.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Core/Logging.h>
#include <Core/Memory.h>

template<typename T>
class CStack
{
public:
	CStack()
		: m_Count{ 0 }
		, m_Capacity{ 0 }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_pData{ nullptr } {}

	CStack(size_t capacity)
		: m_Count{ 0 }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_pData{ nullptr }
	{
		m_pData = gpMemorySystem->MemAllocate(m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_UNKNOWN);
	}

	virtual ~CStack()
	{
		if (m_pData)
			gpMemorySystem->MemFree(m_pData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_STACK);

		m_pData = nullptr;
	}

	T& operator[] (int index)
	{
		return m_pData[index];
	}

	T& Top() const { return m_pData[0]; }

	bool Empty() const { return m_Capacity == 0 && m_Count == 0 && m_pData == nullptr; }

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

	void Push(const T& value)
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

	void Pop()
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

	T* data() { return m_pData; }

private:
	/* The current number of elements in the stack (not necessarily the capacity) */
	size_t m_Count;

	/* The maximum number of elements that can be held in the stack. (Can be resized, of course) */
	size_t m_Capacity;

	/* The stride of the elements (the length in bytes between each element) */
	size_t m_Stride;

	/* The multiplier to be used when the stack resizes to make more room. */
	unsigned int m_ResizeFactor;

	/* The data included in the stack, can be accessed with the [] operators */
	T* m_pData;
};