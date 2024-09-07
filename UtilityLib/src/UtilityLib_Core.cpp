//=============================================================================
// Author: Jared Simons
// Date Created: 9/7/2024
// 
// Purpose: Core hooks and callbacks handed in from the consuming library.
//=============================================================================
#include "UtilityLib_Core.h"

#include <Core/Logging.h>

static UtilLibMemory* s_UtilityLibMemCallbacks = nullptr;
extern UtilLibMemory* gpMemory = s_UtilityLibMemCallbacks;

UtilLibMemory::UtilLibMemory(MemoryAllocate allocFunc, MemoryFree freeFunc)
	: m_AllocateCallback{ allocFunc }
	, m_FreeCallback{ freeFunc }
{
	
}

bool ConnectUtilityLib(UtilLibAPIFuncPointers& apiFunctions)
{
	// Make sure all function pointers are valid, and assign them
	if (apiFunctions.memAllocFunc && apiFunctions.memFreeFunc)
	{
		s_UtilityLibMemCallbacks = new UtilLibMemory(apiFunctions.memAllocFunc, apiFunctions.memFreeFunc);
	}
	else
	{
		LOG_WARN("ConnectUtilityLib() no callbacks passed for memory allocation. Allocations will be made from the system level.");
		LOG_WARN("This should be fixed");
	}

	return true;
}

void DisconnectUtilityLib()
{
	delete s_UtilityLibMemCallbacks;
	s_UtilityLibMemCallbacks = nullptr;
}