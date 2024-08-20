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

	ENGINE_API void GetWindowSize(unsigned int* pOutWidth, unsigned int* pOutHeight);

	ENGINE_API double	GetAbsoluteTime();

	//-----------------------------------------------------------------------------
	// Platform memory functions
	// 
	// Do NOT call these directly, instead call through the memory system
	// TODO: Setup memory allocation hooks for external libs.
	//-----------------------------------------------------------------------------
	ENGINE_API void* MemoryAllocate(size_t size);
	ENGINE_API void	MemoryFree(void* memory);
	ENGINE_API void* MemoryReallocate(void* memory, size_t newSize);
	ENGINE_API void* MemoryCopy(void* dest, const void* source, size_t size);
	ENGINE_API void* MemorySet(void* memory, int value, size_t size);
}