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

template<typename T>
class CVector
{
public:
	CVector()
	{
		m_curNumOfElements = 0;
		m_Capacity = 0;
		m_Stride = static_cast<unsigned int>(sizeof(T));
		m_Data = nullptr;
	}

	CVector(int capacity)
	{
		m_curNumOfElements = capacity;
		m_Capacity = capacity;
		m_Stride = static_cast<unsigned int>(sizeof(T));
		m_Data = static_cast<T*>(malloc(sizeof(T) * capacity));
	}

	CVector(T firstIndex)
	{
		m_curNumOfElements = 1;
		m_Capacity = 1;
		m_Stride = static_cast<unsigned int>(sizeof(T));
		m_Data = static_cast<T*>(malloc(sizeof(T) * m_Capacity));
		if (m_Data != nullptr)
		{
			m_Data[0] = firstIndex;
		}
	}

	CVector(int capacity, T initialValue)
	{
		m_curNumOfElements = capacity;
		m_Capacity = capacity;
		m_Stride = static_cast<unsigned int>(sizeof(T));
		m_Data = static_cast<T*>(malloc(sizeof(T) * capacity));
		if (m_Data != nullptr)
		{
			for (int i = 0; i < capacity; i++)
			{
				m_Data[i] = initialValue;
			}
		}
	}

	CVector(CVector& otherVector)
	{
		m_curNumOfElements = otherVector.m_curNumOfElements;
		m_Capacity = otherVector.m_Capacity;
		m_Stride = otherVector.m_Stride;
		m_Data = static_cast<T*>(malloc(sizeof(T) * m_Capacity));
		memcpy(m_Data, otherVector.m_Data, m_Capacity * m_Stride);
	}

	virtual ~CVector()
	{
		free(m_Data);
	}

	T& operator[] (int index)
	{
		return m_Data[index];
	}

	void Resize(int newCapacity, T newValue = 0)
	{
		if (m_Data != nullptr)
		{
			T* oldData = m_Data;
			m_Data = (T*)malloc(m_Stride * newCapacity);
			
			if (m_Data != nullptr)
			{
				memcpy(m_Data, oldData, m_Stride * newCapacity);
			}

			free(oldData);
		}
		else
		{
			m_Data = (T*)malloc(m_Stride * newCapacity);
		}

		if (newCapacity < (int)m_Capacity)
			m_curNumOfElements = newCapacity;

		m_Capacity = newCapacity;
		int newIndex = newCapacity - m_curNumOfElements;

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

	void PushBack(T val)
	{
		if (m_curNumOfElements + 1 > m_Capacity)
		{
			// Need to resize the vector to make space!

			T* oldData = m_Data;
			m_Capacity++;
			m_curNumOfElements++;

			if (m_Data != nullptr)
			{
				Resize(m_Capacity, val);
			}
			else
			{
				Resize(m_Capacity, val);
			}
		}

		if (m_Data != nullptr)
		{
			m_Data[m_curNumOfElements - 1] = val;
		}
	}

	T PopBack()
	{
		T value = m_Data[m_curNumOfElements - 1];
		m_Data[m_curNumOfElements - 1] = -1;
		return value;
	}
	
	unsigned int GetNumElements() const { return m_curNumOfElements; }
	unsigned int GetCapacity() const { return m_Capacity; }
	unsigned int GetStride() const { return m_Stride; }
	T* Data() { return m_Data; }

private:
	/* The current number of elements in the array (not necessarily the capacity) */
	unsigned int m_curNumOfElements;

	/* The maximum number of elements that can be held in the array. (Can be resized, of course) */
	unsigned int m_Capacity;

	/* The stride of the elements (the length in bytes between each element) */
	unsigned int m_Stride;

	/* The data included in the array, can be accessed with the [] operators */
	T* m_Data;
};