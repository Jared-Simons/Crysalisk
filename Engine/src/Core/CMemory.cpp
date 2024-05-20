#include "Systems/IMemory.h"
#include "Core/Logging.h"

#include "Platform/Platform.h"
#include "EngineGlobals.h"

static CMemorySystem s_MemorySystem;

bool CMemorySystem::MemorySystem_Initialize()
{
	m_numAllocations = 0;

	m_numBytesAllocated = MEBIBYTES(512);

	m_pBaseMemoryBlock = Platform_MemoryAllocate(MEBIBYTES(512));

	LOG_INFO("Memory System initialized with %d bytes of system memory", m_numBytesAllocated);
	return true;
}

void CMemorySystem::MemorySystem_Shutdown()
{
	// Free engine memory on shutdown
	Platform_MemoryFree(m_pBaseMemoryBlock);
	m_numBytesAllocated = 0;

	LOG_INFO("Memory System Shutdown");
}

CMemorySystem* gpMemorySystem = &s_MemorySystem;

