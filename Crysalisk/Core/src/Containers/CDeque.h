//=============================================================================
// Author: Jared Simons
// Date Created: 11/27/2025
// 
// Purpose: A Custom Double-Ended Queue.
//=============================================================================
#pragma once

#include "Memory/IMemoryAllocator.h"
#include "Platform/Platform.h"

namespace Utl
{
	template<typename T>
	class CDeque
	{
	public:
		//--------------------------------------------------
		// Default constructs an empty deque.
		//--------------------------------------------------
		CDeque()
		{
			m_pData = nullptr;
			m_iElementCapacity = 0;
		}

		//--------------------------------------------------
		// Creates a deque with space for 'initialCapacity' number of elements.
		//--------------------------------------------------
		CDeque(u64 initialCapacity)
		{
			// Allocate new memory and place the objects in there.
			void* pNewMemory = nullptr;
			if (m_Allocator != nullptr)
			{
				pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(initialCapacity * sizeof(T)));
			}
			else
			{
				pNewMemory = new T[initialCapacity];
			}
			m_pData = new (pNewMemory) T[initialCapacity];
			m_iElementCapacity = initialCapacity;
		}

		//--------------------------------------------------
		// Copy constructor.
		//--------------------------------------------------
		CDeque(const CDeque& pOtherDeque)
			: m_iElementCount{ pOtherDeque.m_iElementCount }
			, m_iElementCapacity{ pOtherDeque.m_iElementCapacity }
			, m_iHeadElement{ pOtherDeque.m_iHeadElement }
			, m_iTailElement{ pOtherDeque.m_iTailElement }
		{
			// Copy over the allocator, in case it's a custom implementation.
			m_Allocator = pOtherVector.m_Allocator;

			if (m_iElementCapacity)
			{
				void* pNewMemory = nullptr;
				if (m_Allocator != nullptr)
				{
					pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
				}
				else
				{
					pNewMemory = new T[m_iElementCapacity];
				}
				m_pData = new (pNewMemory) T[m_iElementCapacity];
				Platform::MemoryCopy(m_pData, pOtherDeque.m_pData, pOtherDeque.m_iElementCount * sizeof(T));
			}
		}

		//--------------------------------------------------
		// Move constructor.
		//--------------------------------------------------
		CDeque(const CDeque&& pOtherDeque)
			: m_iElementCount{ pOtherDeque.m_iElementCount }
			, m_iElementCapacity{ pOtherDeque.m_iElementCapacity }
			, m_iHeadElement{ pOtherDeque.m_iHeadElement }
			, m_iTailElement{ pOtherDeque.m_iTailElement }
		{
			// Copy over the allocator, in case it's a custom implementation.
			m_Allocator = pOtherVector.m_Allocator;

			if (m_iElementCapacity)
			{
				void* pNewMemory = nullptr;
				if (m_Allocator != nullptr)
				{
					pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
				}
				else
				{
					pNewMemory = new T[m_iElementCapacity];
				}
				m_pData = new (pNewMemory) T[m_iElementCapacity];
				Platform::MemoryCopy(m_pData, pOtherDeque.m_pData, pOtherDeque.m_iElementCount * sizeof(T));
			}
		}

		//--------------------------------------------------
		// Copy assignment.
		//--------------------------------------------------
		CDeque& operator=(const CDeque& pOtherDeque)
		{
			m_iElementCount = pOtherDeque.m_iElementCount;
			m_iElementCapacity = pOtherDeque.m_iElementCapacity;
			m_iHeadElement = pOtherDeque.m_iHeadElement;
			m_iTailElement = pOtherDeque.m_iTailElement;

			// Copy over the allocator, in case it's a custom implementation.
			Platform::MemoryCopy(&m_Allocator, &pOtherDeque.m_Allocator, sizeof(Allocator));

			if (m_iElementCapacity)
			{
				void* pNewMemory = nullptr;
				if (m_Allocator != nullptr)
				{
					pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
				}
				else
				{
					pNewMemory = new T[m_iElementCapacity];
				}
				m_pData = new (pNewMemory) T[m_iElementCapacity];
				Platform::MemoryCopy(m_pData, pOtherDeque.m_pData, pOtherDeque.m_iElementCount * sizeof(T));
			}
			return *this;
		}

		//--------------------------------------------------
		// Destructor.
		//--------------------------------------------------
		~CDeque()
		{
			if (m_pData && m_iElementCapacity > 0)
			{
				if (m_Allocator != nullptr)
				{
					m_Allocator->Free(m_pData);
				}
				else
				{
					delete[] m_pData;
				}
				m_pData = nullptr;
				m_iElementCapacity = 0;
			}
		}

		//--------------------------------------------------
		// Clears and releases memory owned by this deque.
		//--------------------------------------------------
		void Clear()
		{
			if (m_pData && m_iElementCapacity > 0)
			{
				if (m_Allocator != nullptr)
				{
					m_Allocator->Free(m_pData);
				}
				else
				{
					delete[] m_pData;
				}
				m_pData = nullptr;
				m_iElementCapacity = 0;
			}
		}

		//--------------------------------------------------
		// Resizes the deque.
		//--------------------------------------------------
		void Resize(u64 iNewCapacity)
		{
			T* pOldBlock = nullptr;
			if (m_pData)
			{
				pOldBlock = m_pData;
			}

			// Allocate a new block.
			void* pNewMemory = nullptr;
			if (m_Allocator != nullptr)
			{
				pNewMemory = m_Allocator->Allocate(iNewCapacity * sizeof(T));
			}
			else
			{
				pNewMemory = new T[iNewCapacity];
			}
			
			m_pData = new (pNewMemory) T[iNewCapacity];

			// Copy over the existing data.
			if (pOldBlock)
			{
				Platform::MemoryCopy(m_pData, pOldBlock, m_iElementCapacity * sizeof(T));
			}

			// Free the old block.
			if (pOldBlock)
			{
				if (m_Allocator != nullptr)
				{
					m_Allocator->Free(pOldBlock);
				}
				else
				{
					delete[] pOldBlock;
				}
			}
			m_iElementCapacity = iNewCapacity;
		}

		//--------------------------------------------------
		// Element indexing.
		//--------------------------------------------------
		inline T& ElementAt(u64 iIndex) const
		{
			return m_pData[iIndex];
		}

		//--------------------------------------------------
		// Same as above, allows for indexing like an array.
		//--------------------------------------------------
		inline T& operator[](u64 index)
		{
			return ElementAt(index);
		}

		inline const T& operator[](u64 index) const
		{
			return ElementAt(index);
		}

		//--------------------------------------------------
		// Adds an element to the end of the deque.
		// Returns the index of the new element.
		//--------------------------------------------------
		int PushBack(const T& pElement)
		{
			m_iTailElement = (m_iTailElement + 1) % m_iElementCapacity;
			Platform::MemoryCopy(&m_pData[m_iTailElement], &pElement, sizeof(T));
			m_iElementCount++;
			return m_iTailElement;
		}

		//--------------------------------------------------
		// Removes (and returns) the element at the end of the queue.
		//--------------------------------------------------
		T PopBack()
		{
			T elementToReturn = m_pData[m_iTailElement];

			// Zero out the popped element.
			Platform::MemorySet(&m_pData[m_iTailElement], 0, sizeof(T));
			
			m_iTailElement--;
			m_iElementCount--;

			return elementToReturn;
		}

		//--------------------------------------------------
		// Removes (and returns) the element at the front of the queue.
		//--------------------------------------------------
		T PopFront()
		{
			T elementToReturn = m_pData[m_iHeadElement];

			// Zero out the popped element.
			Platform::MemorySet(&m_pData[m_iHeadElement], 0, sizeof(T));

			m_iHeadElement = (m_iHeadElement + 1) % m_iElementCapacity;
			m_iElementCount--;

			return elementToReturn;
		}

		//--------------------------------------------------
		// Returns a point to the front element (without removing it).
		//--------------------------------------------------
		const T* PeekFront() const
		{
			T* pElementToReturn = &m_pData[m_iHeadElement];
			return pElementToReturn;
		}

		int		HeadIndex() const { return m_iHeadElement; }

		u64	Count() const { return m_iElementCount; }
		u64	Capacity() const { return m_iElementCapacity; }

	private:
		u64						m_iElementCount{ 0 };
		u64						m_iElementCapacity{ 0 };
		i32						m_iHeadElement{ 0 };
		i32						m_iTailElement{ -1 };

		// NOTE: Not using a CVector because we do not want this to automatically resize.
		IBaseMemoryAllocator*		m_Allocator{};
		T*							m_pData{ nullptr };
	};
}
