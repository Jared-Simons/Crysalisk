//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: A stack data structure.
//=============================================================================
#pragma once

#include "Core/Assertions.h"

namespace Utl 
{
	template<typename T>
	class CStack
	{
	public:
		CStack()
			: m_Count{ 0 }
			, m_Capacity{ 0 }
			, m_ValueStride{ sizeof(T) }
			, m_pValues{ nullptr }
		{
		}

		CStack(u64 capacity)
			: m_Count{ 0 }
			, m_Capacity{ capacity }
			, m_ValueStride{ sizeof(T) }
			, m_pValues{ nullptr }
		{
			m_pValues = new T[capacity];
		}

		~CStack()
		{
			Clear();
		}

		void Resize(u64 newCapacity)
		{
			T* pOldData = m_pValues;
			T* pNewData = new T[newCapacity];

			if (m_pValues)
			{
				Platform::MemoryCopy(pNewData, pOldData, m_ValueStride * m_Count);
			}
			m_Capacity = newCapacity;

			m_pValues = pNewData;

			if (pOldData)
			{
				delete[] pOldData;
				pOldData = nullptr;
			}
		}

		void Clear()
		{
			if (m_pValues)
			{
				delete[] m_pValues;
				m_pValues = nullptr;
			}

			m_Count = 0;
			m_Capacity = 0;
		}

		void Push(T element)
		{
			if (m_Count + 1 > m_Capacity)
			{
				Resize(m_Capacity * 2);
			}

			m_pValues[m_Count] = element;
			m_Count++;
		}

		void Push(T* pElement)
		{
			if (m_Count + 1 > m_Capacity)
			{
				Resize(m_Capacity * 2);
			}

			m_pValues[m_Count] = *pElement;
			m_Count++;
		}

		inline const T Peek() const
		{
			if (!m_pValues)
			{
				CASSERTMSG(0, "Attempted to peek an empty stack!");
				return nullptr;
			}

			return m_pValues[m_Count - 1];
		}

		T Pop()
		{
			T* pRetValue = &m_pValues[m_Count - 1];
			m_Count--;

			return *pRetValue;
		}

		inline u64 Count() const { return m_Count; }
		inline u64 Capacity() const { return m_Capacity; }
		inline T* Data() const { return m_Data; }

	private:
		/* The current number of elements in the stack (not necessarily the capacity) */
		u64 m_Count{ 0 };

		/* The maximum number of elements that can be held in the stack. (Can be resized, of course) */
		u64 m_Capacity{ 0 };

		/* The stride of the elements (the length in bytes between each element) */
		u64 m_ValueStride{ 0 };

		/* The data stored in the stack. */
		T* m_pValues{ nullptr };
	};
}