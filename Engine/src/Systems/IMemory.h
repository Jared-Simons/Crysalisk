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

	MEM_CATEGORY_MAX_CATEGORIES
};

class IMemorySystem
{
public:
	virtual bool	MemorySystem_Initialize(DynamicAllocator& internalAllocator) = 0;
	virtual void	MemorySystem_Shutdown() = 0;

	inline virtual void*	MemAllocate(size_t numBytes, eMemAllocCategories eAllocCategory = eMemAllocCategories::MEM_CATEGORY_UNKNOWN) = 0;
	inline virtual void	MemFree(void* block, size_t size, eMemAllocCategories eAllocCategory) = 0;
	inline virtual void*	MemReallocate(void* memory, size_t newSize) = 0;
	inline virtual void*	MemCopy(void* dest, const void* source, size_t size) = 0;
	inline virtual void*	MemSet(void* memory, int value, size_t size) = 0;

	virtual void	PrintMemoryStats() = 0;
};

ENGINE_API extern IMemorySystem* gpMemorySystem;

