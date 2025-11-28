//=============================================================================
// Author: Jared Simons
// Date Created: 11/27/2025
// 
// Purpose: A Custom Double-Ended Queue.
//=============================================================================
#pragma once

#include "Memory/MemoryAllocatorBase.h"
#include "Platform/Platform.h"

namespace Utl
{
	const int kDequeDefaultCapacity = 10;

	template<typename T, class Allocator = Utl::CBaseMemoryAllocator>
	class CDeque
	{
	public:
		//--------------------------------------------------
		// Default constructs a queue (defaults to 10 elements).
		//--------------------------------------------------
		CDeque()
		{
			// Allocate new memory and place the objects in there.
			void* pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(kDequeDefaultCapacity * sizeof(T)));
			m_pData = new (pNewMemory) T[kDequeDefaultCapacity];
			m_iElementCapacity = kDequeDefaultCapacity;
		}

		//--------------------------------------------------
		// Creates a deque with space for 'initialCapacity' number of elements.
		//--------------------------------------------------
		CDeque(size_t initialCapacity)
		{
			// Allocate new memory and place the objects in there.
			void* pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(initialCapacity * sizeof(T)));
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
			Platform::MemoryCopy(&m_Allocator, &pOtherDeque.m_Allocator, sizeof(Allocator));

			if (m_iElementCapacity)
			{
				void* pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
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
			Platform::MemoryCopy(&m_Allocator, &pOtherDeque.m_Allocator, sizeof(Allocator));

			if (m_iElementCapacity)
			{
				void* pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
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
				void* pNewMemory = reinterpret_cast<T*>(m_Allocator.Allocate(m_iElementCapacity * sizeof(T)));
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
				m_Allocator.Free(m_pData);
			}
		}

		//--------------------------------------------------
		// Element indexing.
		//--------------------------------------------------
		inline T& ElementAt(size_t iIndex) const
		{
			return m_pData[iIndex];
		}

		//--------------------------------------------------
		// Same as above, allows for indexing like an array.
		//--------------------------------------------------
		inline const T& operator[](size_t index) const
		{
			return ElementAt(index);
		}

		//--------------------------------------------------
		// Adds an element to the end of the deque.
		//--------------------------------------------------
		void PushBack(const T& pElement)
		{
			m_pData[m_iTailElement] = pElement;
			m_iTailElement = (m_iTailElement + 1) % m_iElementCapacity;
			m_iElementCount++;
		}

		//--------------------------------------------------
		// Removes (and returns) the element at the end of the queue.
		//--------------------------------------------------
		T PopBack()
		{
			// TODO: This is borked.
			T elementToReturn = m_pData[m_iTailElement];

			// Zero out the popped element.
			Platform::MemorySet(&m_pData[m_iTailElement], 0, sizeof(T));
			
			m_iTailElement--;
			m_iElementCount--;

			return elementToReturn;
		}

		size_t	Count() const { return m_iElementCount; }
		size_t	Capacity() const { return m_iElementCapacity; }

	private:
		size_t						m_iElementCount{ 0 };
		size_t						m_iElementCapacity{ 0 };
		size_t						m_iHeadElement{ 0 };
		size_t						m_iTailElement{ 0 };

		// NOTE: Not using a CVector because we do not want this be automatically resize.
		Allocator					m_Allocator{};
		T*							m_pData{ nullptr };
	};
}
