//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

bool	Platform_Initialize();
bool	Platform_Update();
void	Platform_Shutdown();

bool	Platform_ProcessMessages();

void	Platform_LogMessage(int level, const char* message);

ENGINE_API double	Platform_GetAbsoluteTime();

//-----------------------------------------------------------------------------
// Platform memory functions
// 
// Do NOT call these directly, instead call through the memory manager
//-----------------------------------------------------------------------------
ENGINE_API void*	Platform_MemoryAllocate(size_t size);
ENGINE_API void	Platform_MemoryFree(void* memory);
ENGINE_API void*	Platform_MemoryReallocate(void* memory, size_t newSize);
ENGINE_API void*	Platform_MemoryCopy(void* dest, const void* source, size_t size);
ENGINE_API void*	Platform_MemorySet(void* memory, int value, size_t size);