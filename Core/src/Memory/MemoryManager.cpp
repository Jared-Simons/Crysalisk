//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Core utility memory functions to be used throughout the engine.
//=============================================================================
#include "MemoryManager.h"

#include "Core/Logging.h"
#include "Memory/DynamicAllocator.h"
#include "Platform/Platform.h"

#include <malloc.h>
#include <string.h>

class CMemoryManager : public IMemoryManager
{
public:
	bool			Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock);
	virtual void	Shutdown() override;

	virtual void*	Allocate(size_t sizeInBytes) override;
	virtual void	Free(void* pMemBlock) override;

	virtual bool		IsInitialized() const override { return m_bInitialized; }
	virtual void	PrintMemoryStats() override;

private:
	ApplicationMemoryConfiguration_t m_MemoryConfig;
	bool		m_bInitialized{ false };
	size_t		m_iNumBytesAllocated{ 0 };
	Utl::DynamicAllocator	m_MemoryAllocator;

	void*		m_pEngineMemory;
};

static CMemoryManager s_pMemoryManager;
IMemoryManager* g_pMemoryManager = &s_pMemoryManager;

bool IMemoryManager::CreateMemorySystem(ApplicationMemoryConfiguration_t config)
{
	// Allocate the block of memory from the OS.
	byte* pMemoryBlock = reinterpret_cast<byte*>(Platform::MemoryAllocate(config.totalMemoryBudget));

	// Initialize the manager.
	s_pMemoryManager.Initialize(config, pMemoryBlock);

	return true;
}

bool CMemoryManager::Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock)
{
	m_pEngineMemory = pMemBlock;

	// Setup our internal memory allocator.
	m_MemoryAllocator.InitAllocatorInternals(config.totalMemoryBudget, pMemBlock, true);
	m_bInitialized = true;

	return true;
}

void CMemoryManager::Shutdown()
{
	m_bInitialized = false;
	// Return our memory to the OS.
	Platform::MemoryFree(m_pEngineMemory);
}

void* CMemoryManager::Allocate(size_t sizeInBytes)
{
	//void* pMemBlock = malloc(sizeInBytes + sizeof(AllocHeader_t));
	/*AllocHeader_t* pAllocHeader = reinterpret_cast<AllocHeader_t*>(malloc(sizeInBytes + sizeof(AllocHeader_t)));
	void* pMemBlock = reinterpret_cast<void*>((uint8_t*)pAllocHeader + kAllocHeaderSize);
	if (pAllocHeader)
	{
		pAllocHeader->Size = sizeInBytes;
		m_iNumBytesAllocated += sizeInBytes;
		memset(pMemBlock, 0, sizeInBytes);
	}*/

	void* pMemBlock = m_MemoryAllocator.Allocate(sizeInBytes);
	m_iNumBytesAllocated += sizeInBytes;
	memset(pMemBlock, 0, sizeInBytes);

	return pMemBlock;
}

void CMemoryManager::Free(void* pMemBlock)
{
	if (pMemBlock && m_bInitialized)	// If the memory system is not initialized, then most likely the application is shutting down and we can't free through this path.
	{
		/* Get at the memory header
		AllocHeader_t* pAllocHeader = reinterpret_cast<AllocHeader_t*>((uint8_t*)pMemBlock - kAllocHeaderSize);
		m_iNumBytesAllocated -= pAllocHeader->Size;

		free(pAllocHeader); */

		int bytesFreed = m_MemoryAllocator.Free(pMemBlock);
		m_iNumBytesAllocated -= bytesFreed;
	}
}

void CMemoryManager::PrintMemoryStats()
{
	LOG_INFO("Bytes Allocated: %d", m_iNumBytesAllocated);
}