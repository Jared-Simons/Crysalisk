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
	virtual float	GetPercentAllocated() const override;

private:
	// The configuration for the memory manager.
	ApplicationMemoryConfiguration_t	m_MemoryConfig;
	// Has the memory manager been initialized?
	bool								m_bInitialized{ false };
	// How many bytes have been allocated through here?
	size_t								m_iNumBytesAllocated{ 0 };
	// A freelist allocator used to track allocations.
	Utl::DynamicAllocator				m_MemoryAllocator;

	// Allows for multiple threads to make allocations.
	Platform::ISystemMutex*				m_pAllocMutex{ nullptr };

	// The memory block that allocations are made from. Managed externally.
	void*								m_pEngineMemory;
};

// Memory manager context.
static CMemoryManager s_pMemoryManager;
IMemoryManager* g_pMemoryManager = &s_pMemoryManager;

static const uint16_t kPlatformMemoryMarker = 5929U;

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

	s_pMemoryManager.m_MemoryConfig = config;

	m_pAllocMutex = Platform::CreateSystemMutex();

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
	
	Platform::DestroySystemMutex(m_pAllocMutex);
	
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

	if (!m_bInitialized)
	{
		// NOTE: No mutex here because it isn't available until the memory system has been initialized.
		void* pMemBlock = Platform::MemoryAllocate(sizeInBytes + kAllocHeaderSize);

		// Set the header of the block to a unique ID so we know how to free it properly.
		uint16_t* pHeader = (uint16_t*)pMemBlock;
		*pHeader = kPlatformMemoryMarker;

		return (uint8_t*)pMemBlock + kAllocHeaderSize;
	}

	if (m_pAllocMutex->Lock())
	{
		void* pMemBlock = m_MemoryAllocator.Allocate(sizeInBytes);
		m_iNumBytesAllocated += sizeInBytes;
		memset(pMemBlock, 0, sizeInBytes);
		if (m_bInitialized)
			m_pAllocMutex->Unlock();
		return pMemBlock;
	}

	LOG_ERROR("Memory Manager failed to get mutex lock for memory allocation. Returning nullptr.");
	return nullptr;
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
	if (m_pAllocMutex->Lock())
	{
		if (pMemBlock && m_bInitialized)	// If the memory system is not initialized, then most likely the application is shutting down and we can't free through this path.
		{
			CAssert(pMemBlock != nullptr);
			int bytesFreed = m_MemoryAllocator.Free(pMemBlock);
			if (bytesFreed < 1)
			{
				// Check the header to see if we need to go through the platform layer.
				if (*((uint16_t*)pMemBlock + kAllocHeaderSize) == kPlatformMemoryMarker)
				{
					Platform::MemoryFree(pMemBlock);
					pMemBlock = nullptr;
					return;
				}
				return;
			}

			m_iNumBytesAllocated -= bytesFreed;
			m_pAllocMutex->Unlock();
			return;
		}
	}

	Platform::MemoryFree(pMemBlock);

	m_pAllocMutex->Unlock();

	// While this doesn't really matter as the OS will handle cleanup,
	// if we're managing memory properly, we should never hit this.
	//CAssert(0);
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

float CMemoryManager::GetPercentAllocated() const
{
	return m_iNumBytesAllocated / (float)m_MemoryConfig.totalMemoryBudget;
}
