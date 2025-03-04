//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: A data structure that stores data in key-value pairs.
// Lookups are done with hashing (duh!)
// 
// TODO: Make this work with custom allocators.
// TODO: Create an iterator for use with range-based for loops.
// BIG TODO: This currently only hashes strings to ints, we need a more generic container.
//=============================================================================
#pragma once

#include "Core/Assertions.h"
#include "Memory/MemoryManager.h"
#include "Utils/Utils.h"

namespace Utl {

	template<typename K, typename V>
	class CHashMap
	{
	public:
		using Key = K;
		using Value = V;

		CHashMap()
			: m_Capacity{ 0 }
			, m_ValueStride{ sizeof(Value) }
			, m_KeyStride{ sizeof(Key) }
			, m_pKeys{ nullptr }
			, m_pValues{ nullptr }
		{
			// Default constructor allocates nothing.
		}

		CHashMap(size_t initialCapacity)
			: m_Capacity{ initialCapacity }
			, m_ValueStride{ sizeof(Value) }
			, m_pKeys{ nullptr }
			, m_pValues{ nullptr }
		{
			// Allocate space for our data.
			m_pKeys = (Key*)new Key[m_Capacity];
			m_pValues = (Value*) new Value[m_Capacity];
		}

		~CHashMap()
		{
			if (m_pKeys)
				delete[] m_pKeys;

			if (m_pValues)
				delete[] m_pValues;
		}

		Value& operator[](Key pKey)
		{
			int hashedKey = SimpleHash_StringToInt(pKey, m_Capacity);
			return m_pValues[hashedKey];
		}

		/*Value operator[](const Key& pKey)
		{
			Key hashedKey = SimpleHash(pKey, m_Capacity);
			return m_pData[pKey];
		}*/

		Value* Insert(Key key)
		{
			// TODO: We need a generic function here
			int hashedValue = ::SimpleHash_StringToInt(key, m_Capacity);
			CAssert((hashedValue >= 0 && hashedValue < m_Capacity));

			// TODO: Check for collisions.
			m_pKeys[hashedValue] = key;

			return &m_pValues[hashedValue];
		}

		// TODO: This currently invalidates the whole container... fix this!
		void Resize(size_t nNewCapacity)
		{
			// Allocate space for our data.
			// TODO: Delete old data.
			m_pKeys = (Key*)new Key[nNewCapacity];
			m_pValues = (Value*) new Value[nNewCapacity];
			m_Capacity = nNewCapacity;
		}

		// This allows us to access our elements in an array like manner.
		Value& ValueElementAt(int index)
		{
			return m_pValues[index];
		}

	private:
		size_t	m_Capacity;
		size_t m_KeyStride;
		size_t m_ValueStride;
		Key* m_pKeys;
		Value* m_pValues;
	};

}