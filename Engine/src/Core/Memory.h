//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Some utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

#include "Defines.h"

// Foward Declarations
class DynamicAllocator;

// NOTE: Any block of memory allocated directly from the platform layer (thus bypassing the core memory system), will be tagged with this magic number
const u32 magicMemPlatformNumber = 0xbadea; 

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

const i32 MAX_MEM_CATEGORIES = static_cast<i32>(eMemAllocCategories::MEM_CATEGORY_MAX_CATEGORIES);

class Memory
{
public:
	static bool		Initialize(DynamicAllocator* pinternalAllocator, void* pState);
	static void		Shutdown(void* pState);

	static void*	Allocate(u64 iNumBytes, eMemAllocCategories eAllocCategory);
	static void		Free(void* pBlock, u64 iNumBytes, eMemAllocCategories eAllocCategory);
	static void*	Reallocate(void* pBlock, u64 iNewSize);
	static void*	MemCopy(void* pDest, const void* pSource, u64 iSize);
	static void*	MemSet(void* pBlock, i32 iValue, u64 iSize);

	static void		PrintMemoryStats();

private:
	u64					m_iNumAllocations;
	u64					m_iNumBytesAllocated;
	u64					m_iTaggedAllocations[MAX_MEM_CATEGORIES];
	DynamicAllocator*	m_pinternalAllocator;
};