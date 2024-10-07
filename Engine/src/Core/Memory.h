//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Some utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

// Foward Declarations
class DynamicAllocator;

// NOTE: Any block of memory allocated directly from the platform layer (thus bypassing the core memory system), will be tagged with this magic number
const unsigned int magicMemPlatformNumber = 0xbadea; 

enum class eMemAllocCategories
{
	MEM_CATEGORY_UNKNOWN,
	MEM_CATEGORY_ARRAY,
	MEM_CATEGORY_CVECTOR,
	MEM_CATEGORY_LIST,
	MEM_CATEGORY_QUEUE,
	MEM_CATEGORY_STACK,
	MEM_CATEGORY_SET,
	MEM_CATEGORY_STRING,
	MEM_CATEGORY_LINEAR_ALLOC,
	MEM_CATEGORY_ENGINE,
	MEM_CATEGORY_GAME,
	MEM_CATEGORY_RENDERER,
	MEM_CATEGORY_TEXTURE,

	MEM_CATEGORY_MAX_CATEGORIES
};

const int MAX_MEM_CATEGORIES = static_cast<int>(eMemAllocCategories::MEM_CATEGORY_MAX_CATEGORIES);

class Memory
{
public:
	static bool		Initialize(DynamicAllocator* pInternalAllocator, void* pState);
	static void		Shutdown(void* pState);

	static void*	Allocate(size_t iNumBytes, eMemAllocCategories eAllocCategory);
	static void		Free(void* pBlock, size_t iNumBytes, eMemAllocCategories eAllocCategory);
	static void*	Reallocate(void* pBlock, size_t iNewSize);
	static void*	MemCopy(void* pDest, const void* pSource, size_t iSize);
	static void*	MemSet(void* pBlock, int iValue, size_t iSize);

	static void			PrintMemoryStats();

private:
	unsigned long long	m_iNumAllocations;
	size_t				m_iNumBytesAllocated;
	size_t				m_iTaggedAllocations[MAX_MEM_CATEGORIES];
	DynamicAllocator*	m_pInternalAllocator;
};