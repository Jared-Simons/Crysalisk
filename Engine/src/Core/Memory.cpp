#include "Memory.h"

#include "Core/Logging.h"

static CMemorySystem s_MemorySystem;

bool CMemorySystem::MemorySystem_Initialize()
{
	m_numAllocations = 0;

	LOG_INFO("Memory System Initialized");
	return true;
}

void CMemorySystem::MemorySystem_Shutdown()
{
	LOG_INFO("Memory System Shutdown");
}

CMemorySystem* g_pMemorySystem = &s_MemorySystem;