#include "Defines.h"
#include "Core/Memory.h"

#include "Core/Logging.h"
#include "Platform/Platform.h"

#include <Memory/DynamicAllocator.h>
#include <String_Utils.h>

static const char* s_MemCategoryStrings[MAX_MEM_CATEGORIES]
{
	"UNKNOWN		",
	"ARRAY			",
	"CVECTOR		",
	"LIST			",
	"QUEUE			",
	"STACK			",
	"SET			",
	"STRING			",
	"LINEAR ALLOC		",
	"ENGINE			",
	"GAME			",
	"RENDERER		",
	"TEXTURE		"
};

class CMemory;

static CMemory* s_MemorySystem = nullptr;
IMemory* gpMemory = nullptr;

class CMemory : public IMemory
{
public:
	void* operator new(size_t size, void* pMemBlock)
	{
		if (pMemBlock)
		{
			return pMemBlock;
		}

		return nullptr;
	}

	// NOTE: This does nothing and just exists to clear a compiler warning, 
	// the memory was allocated and will be freed when the linear allocator gets cleared.
	// If the creation of this system fails, the engine will shutdown regardless.
	void	operator delete(void* pMemory) {}

	bool		Initialize(DynamicAllocator* pInternalAllocator) override;
	void		Shutdown(void* pState) override;

	void*		Allocate(u64 iNumBytes, eMemAllocCategories eAllocCategory) override;
	void		Free(void* pBlock, u64 iNumBytes, eMemAllocCategories eAllocCategory) override;
	void*		Reallocate(void* pBlock, u64 iNewSize) override;
	void*		MemCopy(void* pDest, const void* pSource, u64 iSize) override;
	void*		MemSet(void* pBlock, i32 iValue, u64 iSize) override;

	virtual void		PrintMemoryStats();

private:
	u64					m_iNumAllocations;
	u64					m_iNumBytesAllocated;
	u64					m_iTaggedAllocations[MAX_MEM_CATEGORIES];
	DynamicAllocator* m_pInternalAllocator;
	bool				m_bSystemInitialized;
};

u64 IMemory::m_MemorySystemRequirement = sizeof(CMemory);

IMemory* IMemory::Create(void* pMemoryBlock)
{
	if (s_MemorySystem != nullptr)
	{
		LOG_WARN("Memory::Create -> A memory system is already initialized! Nothing was done");
		return nullptr;
	}

	s_MemorySystem = new (pMemoryBlock) CMemory();
	return s_MemorySystem;
}

bool CMemory::Initialize(DynamicAllocator* pInternalAllocator)
{
	if (!pInternalAllocator)
	{
		LOG_ERROR("Memory System REQUIRES a valid pointer to pInternalAllocator.");
		return false;
	}

	s_MemorySystem->m_iNumAllocations = 0;
	s_MemorySystem->m_iNumBytesAllocated = 0;

	for (i32 index = 0; index < MAX_MEM_CATEGORIES; index++)
	{
		s_MemorySystem->m_iTaggedAllocations[index] = 0;
	}

	s_MemorySystem->m_pInternalAllocator = pInternalAllocator;

	LOG_INFO("Runtime memory system initialized with %d bytes of system memory.", s_MemorySystem->m_pInternalAllocator->GetAllocatorSize());
	s_MemorySystem->m_bSystemInitialized = true;
	return true;
}

void CMemory::Shutdown(void* pState)
{
	if (pState)
	{
		CMemory* pMemorySystem = reinterpret_cast<CMemory*>(pState);
		pMemorySystem->m_bSystemInitialized = false;

		pMemorySystem->m_iNumAllocations -= 1;
		pMemorySystem->m_iNumBytesAllocated -= sizeof(CMemory);

		pMemorySystem->m_pInternalAllocator->Free(pState);

		LOG_INFO("Memory System Shutdown");
	}

	s_MemorySystem = nullptr;
	gpMemory = nullptr;
}

void* CMemory::Allocate(u64 iNumBytes, eMemAllocCategories eAllocCategory)
{
	if (eAllocCategory == eMemAllocCategories::MEM_CATEGORY_UNKNOWN)
	{
		LOG_WARN("Memory->Alloc() - Category is 'UNKNOWN', re-class this allocation.");
	}

	// TODO: Memory alignment
	void* pBlock = nullptr;

	pBlock = s_MemorySystem->m_pInternalAllocator->Allocate(iNumBytes);

	if (pBlock != nullptr)
	{
		s_MemorySystem->m_iNumAllocations++;
		s_MemorySystem->m_iNumBytesAllocated += iNumBytes;
		s_MemorySystem->m_iTaggedAllocations[static_cast<u64>(eAllocCategory)] += iNumBytes;

		return pBlock;
	}

	LOG_ERROR("Memory->Allocate could not allocate memory, returning nullptr.");
	return nullptr;
}

void CMemory::Free(void* pBlock, u64 iNumBytes, eMemAllocCategories eAllocCategory)
{
	if (!s_MemorySystem->m_bSystemInitialized)
		return;

	// TODO: Memory alignment

	s_MemorySystem->m_iNumAllocations--;
	s_MemorySystem->m_iNumBytesAllocated -= iNumBytes;
	s_MemorySystem->m_iTaggedAllocations[static_cast<u64>(eAllocCategory)] -= iNumBytes;
	
	// There is nothing wrong with calling free() on a nullptr,
	// so no need to check.
	s_MemorySystem->m_pInternalAllocator->Free(pBlock);
	pBlock = nullptr;
}

void* CMemory::Reallocate(void* pBlock, u64 iNewSize)
{
	// TODO: Memory alignment
	void* pNewBlock = nullptr;

	pNewBlock = Platform::MemoryReallocate(pBlock, iNewSize);

	// Free the original block
	Platform::MemoryFree(pBlock);

	if (pNewBlock != nullptr)
		return pNewBlock;

	LOG_ERROR("Memory->Reallocate could not allocate memory, returning nullptr.");
	return nullptr;
}

void* CMemory::MemCopy(void* pDest, const void* pSource, u64 iSize)
{
	if (pDest && pSource && iSize)
		return Platform::MemoryCopy(pDest, pSource, iSize);

	LOG_ERROR("Memory->MemCopy requires all parameters to be valid to operate. Returning nullptr.");
	return nullptr;
}

void* CMemory::MemSet(void* pBlock, i32 iValue, u64 iSize)
{
	if (pBlock)
		return Platform::MemorySet(pBlock, iValue, iSize);

	LOG_ERROR("MemorySystem->MemSet requires a valid pointer to a memory block. Returning nullptr.");
	return nullptr;
}

void CMemory::PrintMemoryStats()
{
	char memStatsString[10000] = "Memory Stats:\n";

	for (u64 i = 0; i < MAX_MEM_CATEGORIES; i++)
	{
		char categoryString[256]{ 0 };

		StringFormat_V(" %s\t: %d B\n", nullptr, categoryString, s_MemCategoryStrings[i], s_MemorySystem->m_iTaggedAllocations[i]);
		StringConcat(memStatsString, categoryString);
	}

	char totalMemUsageString[256]{ 0 };
	u64 totalMemoryUsed = s_MemorySystem->m_pInternalAllocator->GetAllocatorSize() - s_MemorySystem->m_pInternalAllocator->GetFreeSpace();
	f32 totalMemoryUsedPercentage = static_cast<f32>(totalMemoryUsed) / s_MemorySystem->m_pInternalAllocator->GetAllocatorSize();
	StringFormat_V(" Overall Memory Usage: %d B of %d B (%.2f perc.)\n", nullptr, totalMemUsageString, totalMemoryUsed, s_MemorySystem->m_pInternalAllocator->GetAllocatorSize(), totalMemoryUsedPercentage);
	StringConcat(memStatsString, totalMemUsageString);

	char totalAllocationString[256]{ 0 };
	StringFormat_V(" Total Memory Allocations: %d", nullptr, totalAllocationString, s_MemorySystem->m_iNumAllocations);
	StringConcat(memStatsString, totalAllocationString);

	LOG_INFO(memStatsString);
}