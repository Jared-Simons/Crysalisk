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
		, m_pData{ nullptr } {}

	CVector(size_t capacity) 
		: m_Count{ 0 }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_pData = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		
		if (m_pData)
			memset(m_pData, 0, sizeof(T) * capacity);
	}

	CVector(T firstIndex)
		: m_Count{ 1 }
		, m_Capacity{ 1 }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_pData = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		
		if (m_pData != nullptr)
		{
			m_pData[0] = firstIndex;
		}
	}

	CVector(size_t capacity, T initialValue)
		: m_Count{ capacity }
		, m_Capacity{ capacity }
		, m_Stride{ sizeof(T) }
		, m_ResizeFactor{ 2 }
	{
		m_pData = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		if (m_pData != nullptr)
		{
			for (int i = 0; i < capacity; i++)
			{
				m_pData[i] = initialValue;
			}
		}
	}

	CVector(CVector& otherVector)
		: m_Count{ otherVector.m_Count }
		, m_Capacity{ otherVector.m_Capacity }
		, m_Stride{ otherVector.m_Stride }
		, m_ResizeFactor{ otherVector.m_ResizeFactor }
	{
		m_pData = static_cast<T*>(gpMemorySystem->MemAllocate(sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR));
		if (m_pData != nullptr)
			memcpy(m_pData, otherVector.m_pData, m_Capacity * m_Stride);
	}

	virtual ~CVector()
	{
		if(m_pData)
			gpMemorySystem->MemFree(m_pData, sizeof(T) * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);

		m_pData = nullptr;
	}

	T& operator[] (int index)
	{
		return m_pData[index];
	}

	void Resize(size_t newCapacity)
	{
		if (m_pData != nullptr)
		{
			T* oldData = m_pData;
			m_pData = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);

			if (m_pData != nullptr)
			{
				memcpy(m_pData, oldData, m_Stride * newCapacity);
			}

			gpMemorySystem->MemFree(oldData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}
		else
		{
			m_pData = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}


		m_Count = newCapacity;
		m_Capacity = newCapacity;
	}

	void Resize(size_t newCapacity, T newValue)
	{
		if (m_pData != nullptr)
		{
			T* oldData = m_pData;

			m_pData = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
			
			if (m_pData != nullptr)
			{
				memcpy(m_pData, oldData, m_Stride * newCapacity);
			}

			gpMemorySystem->MemFree(oldData, m_Stride * m_Capacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}
		else
		{
			m_pData = (T*)gpMemorySystem->MemAllocate(m_Stride * newCapacity, eMemAllocCategories::MEM_CATEGORY_CVECTOR);
		}

		if (newCapacity < (int)m_Capacity)
			m_Count = newCapacity;

		m_Capacity = newCapacity;
		int newIndex = static_cast<int>(newCapacity - m_Count);

		for (int i = (int)m_Capacity - newIndex; i < newCapacity; i++)
		{
			if (m_pData != nullptr)
			{
				m_pData[i] = newValue;
			}
		}
	}

	T& Begin()
	{
		return m_pData[0];
	}

	T& End()
	{
		return m_pData[m_Capacity - 1];
	}

	bool Empty()
	{
		return (m_Count == 0) && (m_Capacity == 0) && (m_pData == nullptr);
	}

	T& index(int index)
	{
		return m_pData[index];
	}

	void PushBack(T val)
	{
		if (m_Count + 1 > m_Capacity)
		{
			// Need to resize the vector to make space!
			T* oldData = m_pData;

			if (m_pData != nullptr)
			{
				Resize(m_Capacity + 1, val);
			}
			else
			{
				Resize(m_Capacity + 1, val);
			}
		}
		m_Count++;

		if (m_pData != nullptr)
		{
			m_pData[m_Count - 1] = val;
		}
	}

	T PopBack()
	{
		T value = m_pData[m_Count - 1];
		m_pData[m_Count - 1] = -1;
		return value;
	}

	void Insert(size_t position, T value)
	{
		if (m_pData[position])
		{
			m_pData[position] = value;
		}
	}

	void SetResizeFactor(unsigned int resizeFactor) { m_ResizeFactor = resizeFactor; }
	
	size_t			size() const { return m_Count; }
	size_t			capacity() const { return m_Capacity; }
	size_t			stride() const { return m_Stride; }
	unsigned int	GetResizeFactor() const { retur m_ResizeFactor; }
	T*				data() { return m_pData; }

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
	T* m_pData;
};