//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

bool	Platform_Initialize();
bool	Platform_Update();
void	Platform_Shutdown();

bool	Platform_ProcessMessages();

void	Platform_LogMessage(int level, const char* message);

//-----------------------------------------------------------------------------
// Platform memory functions
// 
// Do NOT call these directly, instead call through the memory manager
//-----------------------------------------------------------------------------
void*	Platform_MemoryAllocate(size_t size);
void	Platform_MemoryFree(void* memory);
void*	Platform_MemoryReallocate(void* memory, size_t newSize);
void*	Platform_MemoryCopy(void* dest, const void* source, size_t size);
void*	Platform_MemorySet(void* memory, int value, size_t size);