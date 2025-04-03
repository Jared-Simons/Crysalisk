//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Core utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

#include <Defines.h>

struct ApplicationMemoryConfiguration_t
{
	size_t totalMemoryBudget;	// The amount of memory to request from the OS on initialization.
};

class CORE_API IMemoryManager
{
public:
	// NOTE: This static method exists so we can stand up the memory system
	// WITHOUT exposing the implementation to the application. (as a pure virtual method cannot be static).
	// This should not matter though because only one instance of the memory system should exist at a time.
	//
	// This should be among the first things that gets set up (and should happen prior to any dynamic memory allocation).
	static bool			CreateMemorySystem(ApplicationMemoryConfiguration_t config);

	virtual void		Shutdown() = 0;

	virtual void*		Allocate(size_t sizeInBytes) = 0;	// TODO: Memory-Tagging?
	virtual void		Free(void* pMemBlock) = 0;

	virtual void		PrintMemoryStats() = 0;
};

CORE_API extern IMemoryManager* g_pMemoryManager;