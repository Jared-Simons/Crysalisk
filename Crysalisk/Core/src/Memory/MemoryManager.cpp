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

//-------------------------------------------------------------------------
// Memory manager implementation.
//-------------------------------------------------------------------------
class CMemoryManager : public IMemoryManager
{
public:
	bool			Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock);
	virtual void	Shutdown() override;

	virtual void*	Allocate(size_t sizeInBytes) override;
	virtual void	Free(void* pMemBlock) override;

	virtual bool	IsInitialized() const override { return m_bInitialized; }
	virtual void	PrintMemoryStats() override;

private:
	// The configuration for the memory manager.
	ApplicationMemoryConfiguration_t	m_MemoryConfig;
	// Has the memory manager been initialized?
	bool								m_bInitialized{ false };
	// How many bytes have been allocated through here?
	size_t								m_iNumBytesAllocated{ 0 };
	// A freelist allocator used to track allocations.
	Utl::DynamicAllocator				m_MemoryAllocator;

	// The memory block that allocations are made from. Managed externally.
	void*								m_pEngineMemory;
};

// Memory manager context.
static CMemoryManager s_pMemoryManager;
IMemoryManager* g_pMemoryManager = &s_pMemoryManager;

//-------------------------------------------------------------------------
// Purpose: A static method to 'stand up' the global instance of the memory system.
//			Allocates the memory to be used with this system.
// 
// Input:  config - A configuration struct to be passed by the calling application.
//					Configures the memory manager.
// Output: bool - True on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool IMemoryManager::CreateMemorySystem(ApplicationMemoryConfiguration_t config)
{
	// Allocate the block of memory from the OS.
	byte* pMemoryBlock = reinterpret_cast<byte*>(Platform::MemoryAllocate(config.totalMemoryBudget));

	// Initialize the manager.
	s_pMemoryManager.Initialize(config, pMemoryBlock);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Initializes the internal state of the memory manager.
// 
// Input:  config - A configuration struct to be passed by the calling application.
//					Configures the memory manager.
//		   pMemBlock - The memory block that this manager (and by extension,
//					the whole application), will make allocations from.
// 
// Output: bool - True on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool CMemoryManager::Initialize(ApplicationMemoryConfiguration_t config, void* pMemBlock)
{
	m_pEngineMemory = pMemBlock;

	// Setup our internal memory allocator.
	m_MemoryAllocator.InitAllocatorInternals(config.totalMemoryBudget, pMemBlock, true);
	m_bInitialized = true;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the memory manager.
// 
// Input:  none.
// Output: none.
//-------------------------------------------------------------------------
void CMemoryManager::Shutdown()
{
	m_bInitialized = false;
	// Return our memory to the OS.
	Platform::MemoryFree(m_pEngineMemory);
}

//-------------------------------------------------------------------------
// Purpose: Allocates a block of memory from the allocator.
// 
// Input:  sizeInBytes - The size in bytes of the allocation to make.
//				Must be > 0!
// Output: A pointer to the allocated memory.
//-------------------------------------------------------------------------
void* CMemoryManager::Allocate(size_t sizeInBytes)
{
	CAssert(sizeInBytes > 0);

	void* pMemBlock = m_MemoryAllocator.Allocate(sizeInBytes);
	m_iNumBytesAllocated += sizeInBytes;
	memset(pMemBlock, 0, sizeInBytes);

	return pMemBlock;
}

//-------------------------------------------------------------------------
// Purpose: Frees a block of memory owned by the allocator.
// 
// Input:  pMemoryBlock - A pointer to the base of the block of memory
//				allocated by Allocate(). Must not be nullptr!
// Output: none.
//-------------------------------------------------------------------------
void CMemoryManager::Free(void* pMemBlock)
{
	if (pMemBlock && m_bInitialized)	// If the memory system is not initialized, then most likely the application is shutting down and we can't free through this path.
	{
		CAssert(pMemBlock != nullptr);
		int bytesFreed = m_MemoryAllocator.Free(pMemBlock);
		m_iNumBytesAllocated -= bytesFreed;
		return;
	}

	Platform::MemoryFree(pMemBlock);

	// TODO: Handle these issues gracefully.
	CAssert(0);
}

//-------------------------------------------------------------------------
// Purpose: Logs some stats about the current state of memory usage in the application.
// 
// Input:  none.
// Output: none.
//-------------------------------------------------------------------------
void CMemoryManager::PrintMemoryStats()
{
	LOG_INFO("Bytes Allocated: %d", m_iNumBytesAllocated);
}