//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

// Forward Declarations
class LinearAllocator;

namespace Platform {

	bool	Initialize(LinearAllocator* pAllocator);
	bool	Update();
	void	Shutdown();

	bool	ProcessMessages();

	void	LogMessage(int level, const char* message);

	void GetWindowSize(unsigned int* pOutWidth, unsigned int* pOutHeight);

	double	GetAbsoluteTime();

	//-----------------------------------------------------------------------------
	// Platform memory functions
	// 
	// Do NOT call these directly, instead call through the memory system
	// TODO: Setup memory allocation hooks for external libs.
	//-----------------------------------------------------------------------------
	void* MemoryAllocate(size_t size);
	void	MemoryFree(void* memory);
	void* MemoryReallocate(void* memory, size_t newSize);
	void* MemoryCopy(void* dest, const void* source, size_t size);
	void* MemorySet(void* memory, int value, size_t size);
}