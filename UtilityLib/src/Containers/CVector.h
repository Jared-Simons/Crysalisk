//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: A Custom Dynamically-Resizable Array.
//=============================================================================
#pragma once

// TODO: Implement with custom memory system and logging utilities
#include <stdlib.h>
#include <cstring>
#include <stdio.h>

#include <Systems/IMemory.h>
#include <Core/Logging.h>

template<typename T>
class CVector
{
public:
	class iterator
	{

	};

	CVector()
		: m_Count{ 0 }
		, m_Capacity{ 0 }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
		, m_Data{ nullptr } {}

	CVector(size_t capacity)
		: m_Count{ capacity }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_Data = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		
		if (m_Data)
			memset(m_Data, 0, sizeof(T) * capacity);
	}

	CVector(T firstIndex)
		: m_Count{ 1 }
		, m_Capacity{ 1 }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_Data = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		
		if (m_Data != nullptr)
		{
			m_Data[0] = firstIndex;
		}
	}

	CVector(size_t capacity, T initialValue)
		: m_Count{ capacity }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_Data = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		if (m_Data != nullptr)
		{
			for (int i = 0; i < capacity; i++)
			{
				m_Data[i] = initialValue;
			}
		}
	}

	CVector(CVector& otherVector)
		: m_Count{ otherVector.m_Count }
		, m_Capacity{ otherVector.m_Capacity }
		, m_Stride{ otherVector.m_Stride }
		, m_ResizeFactor{ otherVector.m_ResizeFactor }
	{
		m_Data = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		if (m_Data != nullptr)
			memcpy(m_Data, otherVector.m_Data, m_Capacity * m_Stride);
	}

	virtual ~CVector()
	{
		gpMemorySystem->MemFree(m_Data, sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
	}

	T& operator[] (int index)
	{
		return m_Data[index];
	}

	void Resize(size_t newCapacity)
	{
		if (m_Data != nullptr)
		{
			T* oldData = m_Data;
			m_Data = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);

			if (m_Data != nullptr)
			{
				memcpy(m_Data, oldData, m_Stride * newCapacity);
			}

			gpMemorySystem->MemFree(oldData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}
		else
		{
			m_Data = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}


		m_Count = newCapacity;
		m_Capacity = newCapacity;
	}

	void Resize(size_t newCapacity, T newValue)
	{
		if (m_Data != nullptr)
		{
			T* oldData = m_Data;

			m_Data = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
			
			if (m_Data != nullptr)
			{
				memcpy(m_Data, oldData, m_Stride * newCapacity);
			}

			gpMemorySystem->MemFree(oldData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}
		else
		{
			m_Data = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}

		if (newCapacity < (int)m_Capacity)
			m_Count = newCapacity;

		m_Capacity = newCapacity;
		int newIndex = newCapacity - m_Count;

		for (int i = m_Capacity - newIndex; i < newCapacity; i++)
		{
			if (m_Data != nullptr)
			{
				m_Data[i] = newValue;
			}
		}
	}

	T& Begin()
	{
		return m_Data[0];
	}

	T& End()
	{
		return m_Data[m_Capacity - 1];
	}

	bool Empty()
	{
		return (m_Count == 0) && (m_Capacity == 0) && (m_Data == nullptr);
	}

	void PushBack(T val)
	{
		if (m_Count + 1 > m_Capacity)
		{
			// Need to resize the vector to make space!

			T* oldData = m_Data;

			if (m_Data != nullptr)
			{
				Resize(m_Capacity + 1, val);
			}
			else
			{
				Resize(m_Capacity + 1, val);
			}
		}
		m_Count++;

		if (m_Data != nullptr)
		{
			m_Data[m_Count - 1] = val;
		}
	}

	T PopBack()
	{
		T value = m_Data[m_Count - 1];
		m_Data[m_Count - 1] = -1;
		return value;
	}

	void SetResizeFactor(unsigned int resizeFactor) { m_ResizeFactor = resizeFactor; }
	
	size_t			size() const { return m_Count; }
	size_t			capacity() const { return m_Capacity; }
	size_t			stride() const { return m_Stride; }
	unsigned int	GetResizeFactor() const { retur m_ResizeFactor; }
	T*				data() { return m_Data; }

private:
	/* The current number of elements in the array (not necessarily the capacity) */
	size_t m_Count;

	/* The maximum number of elements that can be held in the array. (Can be resized, of course) */
	size_t m_Capacity;

	/* The stride of the elements (the length in bytes between each element) */
	size_t m_Stride;

	/* The multiplier to be used when the vector resizes to make more room. */
	unsigned int m_ResizeFactor;

	/* The data included in the array, can be accessed with the [] operators */
	T* m_Data;
};