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
			m_pKeys = (Key*)g_pMemoryManager->Allocate(m_Capacity * m_KeyStride);
			m_pValues = (Value*)g_pMemoryManager->Allocate(m_Capacity * m_ValueStride);
		}

		~CHashMap()
		{
			if (m_pKeys)
				g_pMemoryManager->Free(m_pKeys);

			if (m_pValues)
				g_pMemoryManager->Free(m_pValues);
		}

		Value& operator[](Key pKey)
		{
			Key hashedKey = SimpleHash(pKey, m_Capacity);
			return (*m_pValues)[hashedKey];
		}

		/*Value operator[](const Key& pKey)
		{
			Key hashedKey = SimpleHash(pKey, m_Capacity);
			return m_pData[pKey];
		}*/

		int Insert(Key key)
		{
			// TODO: We need a generic function here
			Key hashedKey = SimpleHash(value, m_Capacity);
			CAssert((hashedValue >= 0 && hashedValue < m_Capacity));

			// TODO: Check for collisions.
			m_pValues[hashedValue] = value;

			return hashedValue;
		}

		// TODO: This currently invalidates the whole container... fix this!
		void Resize(size_t nNewCapacity)
		{
			m_pKeys = (Key*)g_pMemoryManager->Allocate(sizeof(Key) * nNewCapacity);
			m_pValues = (Value*)g_pMemoryManager->Allocate(sizeof(Value) * nNewCapacity);
		}

	private:
		size_t	m_Capacity;
		size_t m_KeyStride;
		size_t m_ValueStride;
		Key* m_pKeys;
		Value* m_pValues;
	};

}