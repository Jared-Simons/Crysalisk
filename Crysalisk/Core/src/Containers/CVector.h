//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: A Custom Dynamically-Resizable Array.
// 
//=============================================================================
#pragma once

#include "Core/Assertions.h"
#include "Memory/IMemoryAllocator.h"
#include "Memory/MemoryManager.h"
#include "Platform/Platform.h"

#include <new>	// For placement new.

namespace Utl 
{
	//--------------------------------------------------
	// Iterator.
	//--------------------------------------------------
	template<typename T>
	class VectorIterator
	{
	public:
		VectorIterator(T* m_pData) : m_DataPtr{ m_pData }
		{
		}

		bool operator!=(const VectorIterator& pOther)
		{
			return m_DataPtr != pOther.m_DataPtr;
		}

		void operator++()
		{
			m_DataPtr++;
		}

		T operator*()
		{
			return *(m_DataPtr);
		}

	private:
		T* m_DataPtr{ nullptr };
	};

	template<typename T>
	class CVector
	{
		using Iterator = VectorIterator<T>;
	public:
		//--------------------------------------------------
		// Default constructs an empty vector (does not allocate any memory).
		//--------------------------------------------------
		CVector()
			: m_Count{ 0 }
			, m_Capacity{ 0 }
			, m_ValueStride{ sizeof(T) }
			, m_ResizeFactor{ 2 }
			, m_pValues{ nullptr } 
		{
			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}
		}

		//--------------------------------------------------
		// Creates a vector with space for 'initialCapacity' number of elements.
		//--------------------------------------------------
		CVector(u64 initialCapacity, u64 resizeFactor = 2)
			: m_Count{ 0 }
			, m_Capacity{ initialCapacity }
			, m_ValueStride{ sizeof(T) }
			, m_ResizeFactor{ resizeFactor }
			, m_pValues{ nullptr }
		{
			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}

			// Allocate memory for our array.
			if (initialCapacity > 0)
			{
				if (m_Allocator != nullptr)
					m_pValues = reinterpret_cast<T*>(m_Allocator->Allocate(initialCapacity * m_ValueStride));
				else
					m_pValues = new T[initialCapacity];
			}
		}

		//--------------------------------------------------
		// Copy constructor.
		//--------------------------------------------------
		CVector(const CVector& pOtherVector)
			: m_Count{ pOtherVector.m_Count }
			, m_Capacity{ pOtherVector.m_Capacity }
			, m_ValueStride{ pOtherVector.m_ValueStride }
			, m_ResizeFactor{ pOtherVector.m_ResizeFactor }
		{
			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}

			m_Allocator = pOtherVector.m_Allocator;

			if (m_Capacity > 0)
			{
				if (m_Allocator != nullptr)
					m_pValues = reinterpret_cast<T*>(m_Allocator->Allocate(m_Capacity * m_ValueStride));
				else
					m_pValues = new T[m_Capacity];
				
				Platform::MemoryCopy(m_pValues, pOtherVector.m_pValues, pOtherVector.m_ValueStride * pOtherVector.m_Capacity);
			}
		}

		//--------------------------------------------------
		// Move constructor.
		//--------------------------------------------------
		CVector(CVector&& pOtherVector)
			: m_Count{ pOtherVector.m_Count }
			, m_Capacity{ pOtherVector.m_Capacity }
			, m_ValueStride{ pOtherVector.m_ValueStride }
			, m_ResizeFactor{ pOtherVector.m_ResizeFactor }
			/*, m_pValues{pOtherVector.m_pValues} */
		{
			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}

			m_Allocator = pOtherVector.m_Allocator;

			if (m_Capacity > 0)
			{
				if (m_Allocator != nullptr)
					m_pValues = reinterpret_cast<T*>(m_Allocator->Allocate(m_Capacity * m_ValueStride));
				else
					m_pValues = new T[m_Capacity];
				
				Platform::MemoryCopy(m_pValues, pOtherVector.m_pValues, pOtherVector.m_ValueStride * pOtherVector.m_Capacity);
			}
		}

		//--------------------------------------------------
		// I'm a destructor!
		//--------------------------------------------------
		~CVector()
		{
			if (m_pValues && m_Capacity > 0)
			{
				if (m_Allocator != nullptr)
					m_Allocator->Free(m_pValues);
				else
					delete[] m_pValues;
			}
			m_pValues = nullptr;

			m_Count = 0;
			m_Capacity = 0;
		}

		//--------------------------------------------------
		// Copy assignment.
		//--------------------------------------------------
		CVector& operator=(const CVector& pOtherVector)
		{
			m_Count = pOtherVector.m_Count;
			m_Capacity = pOtherVector.m_Capacity;
			m_ValueStride = pOtherVector.m_ValueStride;
			m_ResizeFactor = pOtherVector.m_ResizeFactor;

			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}

			m_Allocator = pOtherVector.m_Allocator;

			if (m_Capacity > 0)
			{
				if (m_Allocator != nullptr)
					m_pValues = reinterpret_cast<T*>(m_Allocator->Allocate(m_Capacity * m_ValueStride));
				else
					m_pValues = new T[m_Capacity];
				
				Platform::MemoryCopy(m_pValues, pOtherVector.m_pValues, pOtherVector.m_ValueStride * pOtherVector.m_Capacity);
			}

			return *this;
		}

		//--------------------------------------------------
		// Sets the allocator this container allocates from
		//--------------------------------------------------
		inline void SetAllocator(IBaseMemoryAllocator* pAllocator)
		{
			CAssert(pAllocator != nullptr);
			CAssert(m_Count == 0 && m_Capacity == 0); // We can't change allocators if we already have some memory allocated.
			m_Allocator = pAllocator;
		}

		//--------------------------------------------------
		// Array indexing.
		//--------------------------------------------------
		inline T& ElementAt(u64 index) const
		{
			CASSERTMSG(index >= 0 && index < m_Capacity, "Attempted to index CVector data out of bounds!");

			// If we happen to index out of bounds, just return the base of the array.
			// Not ideal nor correct under any circumstance, but MUCH better than crashing in release mode.
			if (index < 0 || index >= m_Capacity)
				return m_pValues[0];

			return m_pValues[index];
		}

		//--------------------------------------------------
		// Same as above, for ease of use.
		//--------------------------------------------------
		inline T& operator[](u64 index)
		{
			return ElementAt(index);
		}

		inline T& operator[](u64 index) const
		{
			return ElementAt(index);
		}

		inline T* operator->()
		{
			return m_pValues;
		}

		inline T* operator->() const
		{
			return m_pValues;
		}

		//--------------------------------------------------
		// Access the first element in the array.
		//--------------------------------------------------
		inline T& Front()
		{
			return m_pValues[0];
		}

		inline const T& Front() const
		{
			return m_pValues[0];
		}

		// These exist for us to be able to use this container with an iterator.
		//--------------------------------------------------
		// Returns an iterator to the first element in the container.
		//--------------------------------------------------
		Iterator begin()
		{
			Iterator it(m_pValues);
			return it;
		}

		//--------------------------------------------------
		// Returns an iterator to the last element in the container.
		//--------------------------------------------------
		Iterator end()
		{
			Iterator it(m_pValues + m_Count);

			return it;
		}

		//--------------------------------------------------
		// Access the last element in the array.
		//--------------------------------------------------
		inline T& Back()
		{
			CASSERTMSG(m_pValues != nullptr && m_Count > 0, "Called Back() on an empty CVector!");
			return m_pValues[m_Count - 1];
		}

		const inline T& Back() const
		{
			CASSERTMSG(m_pValues != nullptr && m_Count > 0, "Called Back() on an empty CVector!");
			return m_pValues[m_Count - 1];
		}

		//--------------------------------------------------
		// Access the underlying array's memory block.
		// If the array is empty, then this returns a nullptr.
		//--------------------------------------------------
		inline T* Data()
		{
			return m_pValues;
		}

		inline const T* Data() const
		{
			return m_pValues;
		}

		//--------------------------------------------------
		// Clears the contents within the vector.
		//--------------------------------------------------
		void Clear()
		{
			if (m_pValues && m_Allocator != nullptr)
			{
				if (m_Allocator->GetAllocatorType() != ALLOCATOR_TYPE_LINEAR)	// Don't release memory if this is a linear allocator.
				{
					m_Allocator->Free(m_pValues);
					m_pValues = nullptr;
					m_Capacity = 0;
				}
			}
			else if (m_Allocator == nullptr && m_pValues)	// no custom allocator
			{
				delete[] m_pValues;
				m_pValues = nullptr;
				m_Capacity = 0;
			}

			m_Count = 0;
		}

		//--------------------------------------------------
		// Adds an element to the end of the vector, 
		// (and returns the index to the new element).
		//--------------------------------------------------
		u64 PushBack(T element)
		{
			if (m_Count + 1 > m_Capacity)
			{
				if (m_ResizeFactor < 1)
					m_ResizeFactor = 2; // Just a safety in case something got messed up along the way.

				// We need to resize to make space for us.
				u64 newCapacity = (m_Capacity == 0) ? 1 : m_Count;	// NOTE: If we happen to have no memory allocated, then 
				newCapacity *= m_ResizeFactor;						// newCapacity * m_ResizeFactor will == 0. This is just to prevent that.
				Resize(newCapacity);
			}

			m_pValues[m_Count] = element;
			m_Count++;

			// Return the index of the new element.
			return m_Count - 1;
		}

		//--------------------------------------------------
		// Adds an element to the end of the vector, 
		// (and returns the index to the new element).
		//--------------------------------------------------
		u64 PushBack(T* element)
		{
			if (m_Count + 1 > m_Capacity)
			{
				// We need to resize to make space for us.
				u64 newCapacity = (m_Capacity == 0) ? 1 : m_Count;	// NOTE: If we happen to have no memory allocated, then 
				newCapacity *= m_ResizeFactor;						// newCapacity * m_ResizeFactor will == 0. This is just to prevent that.
				Resize(newCapacity);
			}

			m_pValues[m_Count] = *element;
			m_Count++;

			// Return the index of the new element.
			return m_Count - 1;
		}

		//--------------------------------------------------
		// Resizes the container to newCapacity elements,
		// does nothing if newCapacity <= Count().
		//--------------------------------------------------
		void Resize(u64 newCapacity)
		{
			if (newCapacity <= m_Capacity)
			{
				// No need to even attempt this...
				return;
			}

			// Allocate some new memory.
			void* pNewData = nullptr;
			if (m_Allocator)
			{
				pNewData = m_Allocator->Allocate(m_ValueStride * newCapacity);
			}
			else
			{
				pNewData = new T[newCapacity];
			}
			T* pNewArray = new (pNewData) T[newCapacity];	// This is required so that our allocator is always properly initialized.
			CAssert(pNewArray);

			// Copy the current array over.
			Platform::MemoryCopy(pNewArray, m_pValues, m_ValueStride * m_Count);

			// Free the old memory, and assign the new memory.
			if (m_pValues && m_Capacity > 0)
			{
				if (m_Allocator != nullptr)
				{
					m_Allocator->Free(m_pValues);
				}
				else
				{
					delete[] m_pValues;
				}
			}
			m_pValues = pNewArray;

			// Bump up our capacity!
			m_Capacity = newCapacity;
		}

		// Returns true if the container is empty, otherwise false.
		inline bool IsEmpty() const { return m_Count == 0; }

		// Returns the current number of elements in the vector.
		inline u64 Count() const { return m_Count; }
		inline void	  SetCount(u64 count) { m_Count = count; }

		// Returns the number of bytes between elements in the vector.
		inline u64 Stride() const { return m_ValueStride; }

		// Returns the number of elements able to be stored without resizing the container.
		inline u64 Capacity() const { return m_Capacity; }

	private:
		/* The current number of elements in the array (not necessarily the capacity) */
		u64 m_Count;

		/* The maximum number of elements that can be held in the array. (Can be resized, of course) */
		u64 m_Capacity;

		/* The stride of the elements (the length in bytes between each element) */
		u64 m_ValueStride;

		/* The multiplier to be used when the vector resizes to make more room. */
		u64 m_ResizeFactor;

		/* An allocator that can be used to make memory allocations from.
		If not provided by the user, then an allocator to the application's heap will be created.*/
		IBaseMemoryAllocator* m_Allocator{ nullptr };

		/* The data included in the array, can be accessed with the [] operators */
		T* m_pValues;
	};

}