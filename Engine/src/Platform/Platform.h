//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

#include "Defines.h"

// Forward Declarations
class LinearAllocator;
class Engine;

namespace Platform {

	bool	Initialize(LinearAllocator* pAllocator, Engine* pEngine);
	bool	Update();
	void	Shutdown();

	bool	ProcessMessages();

	void	LogMessage(u8 level, const char* message);

	void	GetWindowSize(u32* pOutWidth, u32* pOutHeight);

	f64		GetAbsoluteTime();

	//-----------------------------------------------------------------------------
	// Platform memory functions
	// 
	// Do NOT call these directly, instead call through the memory system
	// TODO: Setup memory allocation hooks for external libs.
	//-----------------------------------------------------------------------------
	void* MemoryAllocate(u64 size);
	void	MemoryFree(void* memory);
	void* MemoryReallocate(void* memory, u64 newSize);
	void* MemoryCopy(void* dest, const void* source, u64 size);
	void* MemorySet(void* memory, i32 value, u64 size);
}