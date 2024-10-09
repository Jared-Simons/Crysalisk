//=============================================================================
// Author: Jared Simons
// Date Created: 9/7/2024
// 
// Purpose: Core hooks and callbacks handed in from the consuming library.
//=============================================================================
#include "UtilityLib_Core.h"

#include "Memory/DynamicAllocator.h"

#include <Core/Logging.h>

static UtilLibMemory* s_UtilityLibMemCallbacks = nullptr;
extern UtilLibMemory* gpMemory = s_UtilityLibMemCallbacks;

void* UtilLibMemory::operator new(size_t size, DynamicAllocator* pAllocator)
{
	if (pAllocator)
	{
		void* pBlock = pAllocator->Allocate(size);
		if (pBlock)
		{
			return pBlock;
		}
		else
		{
			// TODO: Logging
			return nullptr;
		}
	}
	else
	{
		// TODO: Logging
		return nullptr;
	}
}

void UtilLibMemory::operator delete(void* pMemory, DynamicAllocator* pAllocator)
{
	if (pAllocator)
	{
		UtilLibMemory* pMemorySystem = reinterpret_cast<UtilLibMemory*>(pMemory);
		if (pMemorySystem && pMemorySystem->m_pMemoryAllocator == pAllocator)
		{
			pAllocator->Free(pMemory);
		}
		else
		{
			// TODO: Logging
		}
	}
}

UtilLibMemory::UtilLibMemory(DynamicAllocator* pAllocator, const char* pConsumingLibraryName)
{
	if (!pAllocator || !pConsumingLibraryName)
	{
		// TODO: LOGGING
	}

	m_pMemoryAllocator = pAllocator;
	m_szConsumingLibraryName = pConsumingLibraryName;

	LOG_DEBUG("%s->UtilityLib connected!", m_szConsumingLibraryName);
}

bool ConnectUtilityLib(DynamicAllocator* pMemAllocator, const char* pCallingLibraryName = "")
{
	// Make sure all function pointers are valid, and assign them
	if (pMemAllocator)
	{
		s_UtilityLibMemCallbacks = new (pMemAllocator) UtilLibMemory(pMemAllocator, pCallingLibraryName);
	}
	else
	{
		LOG_WARN("%s->ConnectUtilityLib() no callbacks passed for memory allocation. Allocations will be made from the system level.", pCallingLibraryName);
		LOG_WARN("This should be fixed");
	}

	return true;
}

void DisconnectUtilityLib()
{
	s_UtilityLibMemCallbacks->operator delete(s_UtilityLibMemCallbacks, s_UtilityLibMemCallbacks->m_pMemoryAllocator);
	s_UtilityLibMemCallbacks = nullptr;
}