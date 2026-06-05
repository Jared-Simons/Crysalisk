//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Core utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Memory/IMemoryAllocator.h"

// Some configuration parameters for the application-wide memory manager.
struct ApplicationMemoryConfiguration_t
{
	size_t totalMemoryBudget;	// The amount of memory to request from the OS on initialization.
};

//-------------------------------------------------------------------------
// The interface to the memory manager.
//-------------------------------------------------------------------------
class CORE_API IMemoryManager
{
public:
	// NOTE: This static method exists so we can stand up the memory system
	// WITHOUT exposing the implementation to the application. (as a pure virtual method cannot be static).
	// This should not matter though because only one instance of the memory system should exist at a time.
	//
	// This should be among the first things that gets set up (and should happen prior to any dynamic memory allocation).
	static bool			CreateMemorySystem(ApplicationMemoryConfiguration_t config);

	// Shuts down the memory manager.
	virtual void		Shutdown() = 0;

	// Allocate/free memory from the application-wide memory manager.
	virtual void*		Allocate(size_t sizeInBytes) = 0;
	virtual void		Free(void* pMemBlock) = 0;

	virtual bool		IsInitialized() const = 0;
	virtual void		PrintMemoryStats() = 0;
	virtual float		GetPercentAllocated() const = 0;

	// FIXME: Make const
	virtual Utl::IBaseMemoryAllocator* GetMemoryAllocator() = 0;
};

// A global interface pointer to the application's memory manager.
CORE_API extern IMemoryManager* g_pMemoryManager;