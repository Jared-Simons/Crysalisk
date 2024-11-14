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

class IMemory
{
public:
	static IMemory*		Create(void* pMemoryBlock);

	virtual bool		Initialize(DynamicAllocator* pInternalAllocator) = 0;
	virtual void		Shutdown(void* pState) = 0;

	virtual void*		Allocate(u64 iNumBytes, eMemAllocCategories eAllocCategory) = 0;
	virtual void		Free(void* pBlock, u64 iNumBytes, eMemAllocCategories eAllocCategory) = 0;
	virtual void*		Reallocate(void* pBlock, u64 iNewSize) = 0;
	virtual void*		MemCopy(void* pDest, const void* pSource, u64 iSize) = 0;
	virtual void*		MemSet(void* pBlock, i32 iValue, u64 iSize) = 0;
	static u64			GetSystemMemoryRequirement() { return m_MemorySystemRequirement; }

	virtual void		PrintMemoryStats() {};

private:
	static u64			m_MemorySystemRequirement;
};

extern IMemory* gpMemory;