//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Some utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

class IMemorySystem
{
public:
	virtual bool MemorySystem_Initialize() = 0;
	virtual void MemorySystem_Shutdown() = 0;

	virtual void*	MemAllocate(size_t numBytes) = 0;
	virtual void	MemFree(void* block) = 0;
	virtual void*	MemReallocate(void* memory, size_t newSize) = 0;
	virtual void*	MemCopy(void* dest, const void* source, size_t size) = 0;
	virtual void*	MemSet(void* memory, int value, size_t size) = 0;
};

ENGINE_API extern IMemorySystem* gpMemorySystem;

