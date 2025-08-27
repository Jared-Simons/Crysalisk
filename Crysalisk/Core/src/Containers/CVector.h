//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: A Custom Dynamically-Resizable Array.
// 
//=============================================================================
#pragma once

#include "Core/Assertions.h"
#include "Memory/MemoryAllocatorBase.h"
#include "Memory/MemoryManager.h"
#include "Platform/Platform.h"

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

	template<typename T, class Allocator = Utl::CBaseMemoryAllocator>
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
		CVector(size_t initialCapacity, size_t resizeFactor = 2)
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
			//m_pValues = new T[initialCapacity];
			if (initialCapacity > 0)
				m_pValues = reinterpret_cast<T*>(m_Allocator.Allocate(initialCapacity * m_ValueStride));
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

			// Make sure to copy over a custom allocator, if we have one!
			Platform::MemoryCopy(&m_Allocator, &pOtherVector.m_Allocator, sizeof(Allocator));

			if (m_Capacity > 0)
			{
				//m_pValues = new T[m_Capacity];
				m_pValues = reinterpret_cast<T*>(m_Allocator.Allocate(m_Capacity * m_ValueStride));
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
			, m_pValues{ pOtherVector.m_pValues }
		{
			if (m_ResizeFactor == 0)
			{
				m_ResizeFactor = 2; // Never let this be 0!
			}

			// Make sure to copy over a custom allocator, if we have one!
			Platform::MemoryCopy(&m_Allocator, &pOtherVector.m_Allocator, sizeof(Allocator));

			// Release the other vector's referece to these resources.
			pOtherVector.m_pValues = nullptr;
			pOtherVector.m_Count = 0;
			pOtherVector.m_Capacity = 0;
		}

		//--------------------------------------------------
		// I'm a destructor!
		//--------------------------------------------------
		~CVector()
		{
			if (m_pValues && m_Capacity > 0)
				m_Allocator.Free(m_pValues);
				//delete[] m_pValues;	// There is something screwy happening with new and delete where it is not accessing the correct addresses (Perhaps an alignment issue?)
			m_pValues = nullptr;

			m_Count = 0;
			m_Capacity = 0;
			//m_ValueStride = 0;	// We CAN'T set this to 0 in case the user decides they want to re-use this container.
		}

		//--------------------------------------------------
		// Copy assignment
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

			// Make sure to copy over a custom allocator, if we have one!
			Platform::MemoryCopy(&m_Allocator, &pOtherVector.m_Allocator, sizeof(Allocator));

			if (m_Capacity > 0)
			{
				//m_pValues = new T[m_Capacity];	// Some memory alignment requirements are causing this to not work rn :(
				m_pValues = reinterpret_cast<T*>(m_Allocator.Allocate(m_Capacity * m_ValueStride));
				Platform::MemoryCopy(m_pValues, pOtherVector.m_pValues, pOtherVector.m_ValueStride * pOtherVector.m_Capacity);
			}

			return *this;
		}

		//--------------------------------------------------
		// Array indexing.
		//--------------------------------------------------
		inline T& ElementAt(size_t index) const
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
		inline T& operator[](size_t index)
		{
			return ElementAt(index);
		}

		inline T& operator[](size_t index) const
		{
			return ElementAt(index);
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
		// The capacity of the vector remains unchanged.
		//--------------------------------------------------
		void Clear()
		{
			if (m_pValues)
				m_Allocator.Free(m_pValues);	// The overloaded delete[] operator can't seem to be found in here...
			//delete[] m_pValues;
			m_pValues = nullptr;
			m_Capacity = 0;
			m_Count = 0;
		}

		//--------------------------------------------------
		// Adds an element to the end of the vector, 
		// (and returns the index to the new element).
		//--------------------------------------------------
		size_t PushBack(T element)
		{
			if (m_Count + 1 > m_Capacity)
			{
				// We need to resize to make space for us.
				size_t newCapacity = (m_Capacity == 0) ? 1 : m_Count;	// NOTE: If we happen to have no memory allocated, then 
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
		size_t PushBack(T* element)
		{
			if (m_Count + 1 > m_Capacity)
			{
				// We need to resize to make space for us.
				size_t newCapacity = (m_Capacity == 0) ? 1 : m_Count;	// NOTE: If we happen to have no memory allocated, then 
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
		void Resize(size_t newCapacity)
		{
			if (newCapacity <= m_Count)
			{
				// No need to even attempt this...
				return;
			}

			// Allocate some new memory.
			void* pNewData = m_Allocator.Allocate(m_ValueStride * newCapacity);	// There is something screwy happening with new and delete where it is not accessing the correct addresses (Perhaps an alignment issue?)
			T* pNewArray = (T*)pNewData;												// For example, using new[] here seems to be inconsistent with returning a pointer to the correct address allocated, or an address that is some amount of bytes ahead of the allocated address.
			CAssert(pNewArray);

			// Copy the current array over.
			Platform::MemoryCopy(pNewArray, m_pValues, m_ValueStride * m_Count);

			// Free the old memory, and assign the new memory.
			if (m_pValues && m_Capacity > 0)
				m_Allocator.Free(m_pValues);	// The overloaded delete[] operator can't seem to be found in here...
				//delete[] m_pValues;
			m_pValues = reinterpret_cast<T*>(pNewArray);

			// Bump up our capacity!
			m_Capacity = newCapacity;
		}

		// Returns true if the container is empty, otherwise false.
		inline bool IsEmpty() const { return m_Count == 0; }

		// Returns the current number of elements in the vector.
		inline size_t Count() const { return m_Count; }
		inline void	  SetCount(size_t count) { m_Count = count; }

		// Returns the number of bytes between elements in the vector.
		inline size_t Stride() const { return m_ValueStride; }

		// Returns the number of elements able to be stored without resizing the container.
		inline size_t Capacity() const { return m_Capacity; }

	private:
		/* The current number of elements in the array (not necessarily the capacity) */
		size_t m_Count;

		/* The maximum number of elements that can be held in the array. (Can be resized, of course) */
		size_t m_Capacity;

		/* The stride of the elements (the length in bytes between each element) */
		size_t m_ValueStride;

		/* The multiplier to be used when the vector resizes to make more room. */
		size_t m_ResizeFactor;

		/* An allocator that can be used to make memory allocations from.
		If not provided by the user, then an allocator to the application's heap will be created.*/
		Allocator m_Allocator{};

		/* The data included in the array, can be accessed with the [] operators */
		T* m_pValues;
	};

}