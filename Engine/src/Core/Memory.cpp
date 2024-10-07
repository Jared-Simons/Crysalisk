#include "Defines.h"
#include "Core/Memory.h"

#include "Core/Logging.h"
#include "Platform/Platform.h""

#include <Memory/DynamicAllocator.h>
#include <String_Utils.h>

static Memory* s_MemorySystem = nullptr;

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

bool Memory::Initialize(DynamicAllocator* pinternalAllocator, void* pState)
{
	if (!pinternalAllocator)
	{
		LOG_ERROR("Memory System REQUIRES a valid pointer to pinternalAllocator.");
		return false;
	}
	pState = pinternalAllocator->Allocate(sizeof(Memory));
	s_MemorySystem = reinterpret_cast<Memory*>(pState);

	s_MemorySystem->m_iNumAllocations = 1; // We just made one allocation.
	s_MemorySystem->m_iNumBytesAllocated = sizeof(Memory);

	for (int index = 0; index < MAX_MEM_CATEGORIES; index++)
	{
		s_MemorySystem->m_iTaggedAllocations[index] = 0;
	}

	s_MemorySystem->m_iTaggedAllocations[static_cast<int>(eMemAllocCategories::MEM_CATEGORY_ENGINE)] += sizeof(Memory);

	s_MemorySystem->m_pinternalAllocator = pinternalAllocator;

	LOG_INFO("Runtime memory system initialized with %d bytes of system memory.", s_MemorySystem->m_pinternalAllocator->GetAllocatorSize());
	return true;
}

void Memory::Shutdown(void* pState)
{
	if (pState)
	{
		Memory* pMemorySystem = reinterpret_cast<Memory*>(pState);

		pMemorySystem->m_iNumAllocations -= 1;
		pMemorySystem->m_iNumBytesAllocated -= sizeof(Memory);

		pMemorySystem->m_pinternalAllocator->Free(pState);

		LOG_INFO("Memory System Shutdown");
	}

	s_MemorySystem = nullptr;
}

void* Memory::Allocate(u64 iNumBytes, eMemAllocCategories eAllocCategory)
{
	if (eAllocCategory == eMemAllocCategories::MEM_CATEGORY_UNKNOWN)
	{
		LOG_WARN("Memory->Alloc() - Category is 'UNKNOWN', re-class this allocation.");
	}

	// TODO: Memory alignment
	void* pBlock = nullptr;

	pBlock = s_MemorySystem->m_pinternalAllocator->Allocate(iNumBytes);

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

void Memory::Free(void* pBlock, u64 iNumBytes, eMemAllocCategories eAllocCategory)
{
	// TODO: Memory alignment

	s_MemorySystem->m_iNumAllocations--;
	s_MemorySystem->m_iNumBytesAllocated -= iNumBytes;
	s_MemorySystem->m_iTaggedAllocations[static_cast<u64>(eAllocCategory)] -= iNumBytes;
	
	// There is nothing wrong with calling free() on a nullptr,
	// so no need to check.
	s_MemorySystem->m_pinternalAllocator->Free(pBlock);
}

void* Memory::Reallocate(void* pBlock, u64 iNewSize)
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

void* Memory::MemCopy(void* pDest, const void* pSource, u64 iSize)
{
	if (pDest && pSource && iSize)
		return Platform::MemoryCopy(pDest, pSource, iSize);

	LOG_ERROR("Memory->MemCopy requires all parameters to be valid to operate. Returning nullptr.");
	return nullptr;
}

void* Memory::MemSet(void* pBlock, int iValue, u64 iSize)
{
	if (pBlock)
		return Platform::MemorySet(pBlock, iValue, iSize);

	LOG_ERROR("MemorySystem->MemSet requires a valid pointer to a memory block. Returning nullptr.");
	return nullptr;
}

void Memory::PrintMemoryStats()
{
	char memStatsString[10000] = "Memory Stats:\n";

	for (u64 i = 0; i < MAX_MEM_CATEGORIES; i++)
	{
		char categoryString[256]{ 0 };

		StringFormat_V(" %s\t: %d B\n", nullptr, categoryString, s_MemCategoryStrings[i], s_MemorySystem->m_iTaggedAllocations[i]);
		StringConcat(memStatsString, categoryString);
	}

	char totalMemUsageString[256]{ 0 };
	u64 totalMemoryUsed = s_MemorySystem->m_pinternalAllocator->GetAllocatorSize() - s_MemorySystem->m_pinternalAllocator->GetFreeSpace();
	f32 totalMemoryUsedPercentage = static_cast<float>(totalMemoryUsed) / s_MemorySystem->m_pinternalAllocator->GetAllocatorSize();
	StringFormat_V(" Overall Memory Usage: %d B of %d B (%.2f perc.)\n", nullptr, totalMemUsageString, totalMemoryUsed, s_MemorySystem->m_pinternalAllocator->GetAllocatorSize(), totalMemoryUsedPercentage);
	StringConcat(memStatsString, totalMemUsageString);

	char totalAllocationString[256]{ 0 };
	StringFormat_V(" Total Memory Allocations: %d", nullptr, totalAllocationString, s_MemorySystem->m_iNumAllocations);
	StringConcat(memStatsString, totalAllocationString);

	LOG_INFO(memStatsString);
}

