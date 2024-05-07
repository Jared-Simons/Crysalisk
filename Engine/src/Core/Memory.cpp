#include "Systems/IMemory.h"
#include "Core/Logging.h"

#include "Platform/Platform.h"

static CMemorySystem s_MemorySystem;

bool CMemorySystem::MemorySystem_Initialize()
{
	m_numAllocations = 0;

	LOG_INFO("Memory System initialized!");
	return true;
}

void CMemorySystem::MemorySystem_Shutdown()
{
	LOG_INFO("Memory System Shutdown");
}

CMemorySystem* gpMemorySystem = &s_MemorySystem;

