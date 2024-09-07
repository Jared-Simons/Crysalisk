//=============================================================================
// Author: Jared Simons
// Date Created: 9/7/2024
// 
// Purpose: Core hooks and callbacks handed in from the consuming library.
//=============================================================================
#pragma once

// ----------------------------------------------------------------------------
// Memory
// ----------------------------------------------------------------------------
typedef void* (*MemoryAllocate)(size_t size, size_t alignment);
typedef void (*MemoryFree)(void* pMemBlock);

class UtilLibMemory
{
public:
	UtilLibMemory(MemoryAllocate allocFunc, MemoryFree freeFunc);

private:
	MemoryAllocate m_AllocateCallback{ nullptr };
	MemoryFree m_FreeCallback{ nullptr };
};

extern UtilLibMemory* gpMemory;


// ----------------------------------------------------------------------------
// API handling
// ----------------------------------------------------------------------------
struct UtilLibAPIFuncPointers
{
	MemoryAllocate memAllocFunc = nullptr;
	MemoryFree	memFreeFunc = nullptr;
};

// This must be called by EVERY consuming library that wishes to use these utilities.
// Every function pointer MUST be assigned.
bool ConnectUtilityLib(UtilLibAPIFuncPointers& apiFunctions);

// Similarly, this must be called when the consuming library is shutting down.
void DisconnectUtilityLib();
