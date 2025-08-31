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

namespace Utl 
{
	// This is a hashmap that assumes a C-String for the key.
	// This specialization is required because strings are compared differently from other types,
	// and because most of our data is stored using strings for keys anyways.
	template<typename V>
	class CStringHashMap
	{
	public:
		using Value = V;

		CStringHashMap()
			: m_Capacity{ 0 }
			, m_ValueStride{ sizeof(Value) }
			, m_KeyStride{ sizeof(const char*) }
			, m_pKeys{ nullptr }
			, m_pValues{ nullptr }
		{
			// Default constructor allocates nothing.
		}

		CStringHashMap(size_t initialCapacity)
			: m_Capacity{ initialCapacity }
			, m_ValueStride{ sizeof(Value) }
			, m_KeyStride{ sizeof(const char*) }
			, m_pKeys{ nullptr }
			, m_pValues{ nullptr }
		{
			// Allocate space for our data.
			m_pKeys = (const char**)new const char*[m_Capacity];
			m_pValues = (Value*) new Value[m_Capacity];
		}

		~CStringHashMap()
		{
			if (m_pKeys)
				delete[] m_pKeys;

			if (m_pValues)
				delete[] m_pValues;
		}

		Value& operator[](const char* pKey)
		{
			int hashedKey = SimpleHash_StringToInt(pKey, m_Capacity);
			// Handle collisions here.
			if (pKey && m_pKeys[hashedKey] && Utl::StringCompare(m_pKeys[hashedKey], pKey))
			{
				// We're good here.
				return m_pValues[hashedKey];
			}
			else
			{
				// Perform linear probing to attempt to find the right key.
				for (int iProbeIdx = 1; iProbeIdx < m_Capacity; iProbeIdx++)
				{
					if (m_pKeys[(hashedKey + iProbeIdx) % m_Capacity] && Utl::StringCompare(m_pKeys[(hashedKey + iProbeIdx) % m_Capacity], pKey))
					{
						// Found it.
						return m_pValues[(hashedKey + iProbeIdx) % m_Capacity];
					}
				}
			}

			// We couldn't find our element.
			// Attempt to add it to the table.
			Value* pNewKey = Insert(pKey);
			return *pNewKey;
		}

		/*Value operator[](const Key& pKey)
		{
			Key hashedKey = SimpleHash(pKey, m_Capacity);
			return m_pData[pKey];
		}*/

		Value* Insert(const char* key)
		{
			// TODO: We need a generic function here
			int hashedValue = ::SimpleHash_StringToInt(key, m_Capacity);
			CAssert(hashedValue >= 0 && hashedValue < m_Capacity);

			if (m_pKeys[hashedValue] == nullptr)
			{
				m_pKeys[hashedValue] = key;
			}
			else
			{
				// Perform linear probing.
				// This method should be fast enough since we typically store off hashed data for quick lookups anyways.
				for (int iProbeIdx = 1; iProbeIdx < m_Capacity; iProbeIdx++)
				{
					// We can use this slot.
					if (m_pKeys[(hashedValue + iProbeIdx) % m_Capacity] == nullptr)
					{
						m_pKeys[(hashedValue + iProbeIdx) % m_Capacity] = key;
						return &m_pValues[(hashedValue + iProbeIdx) % m_Capacity];
					}
				}
			}
			return &m_pValues[hashedValue];
		}

		// TODO: This currently invalidates the whole container... fix this!
		void Resize(size_t nNewCapacity)
		{
			// Allocate space for our data.
			// TODO: Delete old data.
			m_pKeys = (const char**)new char*[nNewCapacity];
			m_pValues = (Value*) new Value[nNewCapacity];
			m_Capacity = nNewCapacity;
		}

		// This allows us to access our elements in an array like manner.
		Value& ValueElementAt(int index)
		{
			return m_pValues[index];
		}

	private:
		size_t			m_Capacity;
		size_t			m_KeyStride;
		size_t			m_ValueStride;
		const char**	m_pKeys;
		Value*			m_pValues;
	};
} // namespace Utl