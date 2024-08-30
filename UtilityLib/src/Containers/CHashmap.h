//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: A Custom Map data structure implemented with a CHashtable and CVector.
//=============================================================================
#pragma once

#include "CVector.h"
#include "../String_Utils.h"
#include "../Math/MathLib.h"

#include <Core/Logging.h>

template <typename TKey, typename TValue>
class CHashmap
{
public:
	CHashmap();
	CHashmap(size_t mapSize);
	CHashmap(CHashmap& otherMap);

	~CHashmap();

	TValue& operator[] (TKey pKey);

	size_t			size() const { return m_Count; }
	size_t			capacity() const { return m_Capacity; }
	size_t			stride() const { return m_Stride; }

private:
	size_t GetHashFromKey(const char* pString);

private:
	/* The current number of elements in the map (not necessarily the capacity) */
	size_t	m_Count;

	/* The maximum number of elements that can be held in the map. (Can be resized, of course) */
	size_t	m_Capacity;

	/* The stride of the elements (the length in bytes between each element) */
	size_t	m_Stride;

	/* The data included in the map, can be accessed with the [] operators. */
	CVector<TValue>*		m_pData;
};

template <typename TKey, typename TValue>
CHashmap<TKey, TValue>::CHashmap()
	: m_Count{ 0 }, m_Capacity{ 10 }
	, m_Stride{ sizeof(TValue) }, m_pData{}
{
	m_pData = new CVector<TValue>(m_Capacity);
}

template <typename TKey, typename TValue>
CHashmap<TKey, TValue>::CHashmap(size_t mapSize)
	: m_Count{ mapSize }, m_Capacity{ mapSize }
	, m_Stride{ sizeof(TValue)}, m_pData{nullptr}
{
	m_pData = new CVector<TValue>(mapSize);
}

template <typename TKey, typename TValue>
CHashmap<TKey, TValue>::CHashmap(CHashmap& otherMap)
	: m_Count{ otherMap.m_Count }, m_Capacity{ otherMap.m_Capacity }
	, m_Stride{ otherMap.m_Stride }, m_pData{ otherMap.m_pData } 
{
}

template <typename TKey, typename TValue>
CHashmap<TKey, TValue>::~CHashmap()
{
	m_Count = 0;
	m_Capacity = 0;
	m_Stride = 0;
}

template <typename TKey, typename TValue>
TValue& CHashmap<TKey, TValue>::operator[](TKey pKey)
{
	size_t hashedIndex = GetHashFromKey(pKey);
	return m_pData->data()[hashedIndex];
}

// Hashing function borrowed from "https://opendsa-server.cs.vt.edu/ODSA/Books/CS3/html/HashFuncExamp.html"
template <typename TKey, typename TValue>
size_t CHashmap<TKey, TValue>::GetHashFromKey(const char* pString)
{
	int sum = 0, mul = 1;
	int len = StringLength(pString);

	for (int index = 0; index < len; index++)
	{
		mul = (index % 4 == 0) ? 1 : mul * 256;
		sum += pString[index] * mul;
	}

	return static_cast<size_t>((size_t)M_ABS(sum) % m_Capacity);
}