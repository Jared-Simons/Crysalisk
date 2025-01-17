//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Core utility memory functions to be used throughout the engine.
//=============================================================================
#include "MemoryManager.h"

#include "Platform/Platform.h"

//  TODO: This is TEMPORARY!
#include <malloc.h>
#include <string.h>

class CMemoryManager : public IMemoryManager
{
public:
	bool			Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock);
	virtual void	Shutdown() override;

	virtual void*	Allocate(size_t sizeInBytes) override;
	virtual void	Free(void* pMemBlock) override;

private:
	ApplicationMemoryConfiguration_t m_MemoryConfig;
	void* m_pEngineMemory;
};

static CMemoryManager s_pMemoryManager;
IMemoryManager* g_pMemoryManager = &s_pMemoryManager;

bool IMemoryManager::CreateMemorySystem(ApplicationMemoryConfiguration_t config)
{
	// Allocate the block of memory from the OS.
	// TODO: Enable this when we move the platform code to Core.dll
	//byte* pMemoryBlock = reinterpret_cast<byte*>(Platform::MemoryAllocate(config.totalMemoryBudget));
	byte* pMemoryBlock = reinterpret_cast<byte*>(malloc(config.totalMemoryBudget));

	// Initialize the manager.
	s_pMemoryManager.Initialize(config, pMemoryBlock);

	return true;
}

bool CMemoryManager::Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock)
{
	m_pEngineMemory = pMemBlock;

	return true;
}

void CMemoryManager::Shutdown()
{
	// Return our memory to the OS.
	//Platform::MemoryFree(m_pEngineMemory);
	// TODO: Enable this when we move the platform code to Core.dll
	free(m_pEngineMemory);
}

void* CMemoryManager::Allocate(size_t sizeInBytes)
{
	void* pMemBlock = malloc(sizeInBytes);
	memset(pMemBlock, 0, sizeInBytes);
	return pMemBlock;
}

void CMemoryManager::Free(void* pMemBlock)
{
	if (pMemBlock)
		free(pMemBlock);
}
