//=============================================================================
// Author: Jared Simons
// Date Created: 9/7/2024
// 
// Purpose: Core hooks and callbacks handed in from the consuming library.
//=============================================================================
#pragma once

// Forward declarations
class DynamicAllocator;

class UtilLibMemory
{
public:
	void* operator new(size_t size, DynamicAllocator* pAllocator);
	void  operator delete(void* pMemory, DynamicAllocator* pAllocator);

	UtilLibMemory(DynamicAllocator* pAllocator, const char* pConsumingLibraryName);

	// The memory allocator that this instance of UtilLib will allocate from.
	DynamicAllocator* m_pMemoryAllocator{ nullptr };
	const char* m_szConsumingLibraryName{ nullptr };
};

extern UtilLibMemory* gpMemory;

// This must be called by EVERY consuming library that wishes to use these utilities.
// Every function pointer MUST be assigned.
bool ConnectUtilityLib(DynamicAllocator* pMemAllocator, const char* pCallingLibraryName);

// Similarly, this must be called when the consuming library is shutting down.
void DisconnectUtilityLib();
