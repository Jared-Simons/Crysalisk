#include "Systems/IMemory.h"
#include "Core/Logging.h"

#include "Platform/Platform.h"
#include "EngineGlobals.h"

#include <Memory/DynamicAllocator.h>
#include <String_Utils.h>

const size_t MAX_MEM_CATEGORIES = (size_t)eMemAllocCategories::MEM_CATEGORY_MAX_CATEGORIES;
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
	"DYNAMIC ALLOC		",
	"ENGINE			",
	"GAME			",
	"RENDERER		"
};

class CMemorySystem : public IMemorySystem
{
public:
	CMemorySystem()
		: m_numAllocations{ 0 }
		, m_numBytesAllocated{ 0 }
		, m_taggedAllocations{ 0 }
		, m_InternalAllocator{ nullptr } {}

	bool	MemorySystem_Initialize(DynamicAllocator& internalAllocator) override;
	void	MemorySystem_Shutdown() override;

	inline void*	MemAllocate(size_t numBytes, eMemAllocCategories eAllocCategory = eMemAllocCategories::MEM_CATEGORY_UNKNOWN) override;
	inline void		MemFree(void* block, size_t size, eMemAllocCategories eAllocCategory) override;
	inline void*	MemReallocate(void* memory, size_t newSize) override;
	inline void*	MemCopy(void* dest, const void* source, size_t size) override;
	inline void*	MemSet(void* memory, int value, size_t size) override;

	void	PrintMemoryStats() override;

private:
	unsigned int	m_numAllocations;
	size_t			m_numBytesAllocated;
	size_t			m_taggedAllocations[MAX_MEM_CATEGORIES];
	DynamicAllocator* m_InternalAllocator;
};

bool CMemorySystem::MemorySystem_Initialize(DynamicAllocator& internalAllocator)
{
	m_InternalAllocator = &internalAllocator;

	LOG_INFO("Memory System initialized with %d bytes of system memory", m_InternalAllocator->GetAllocatorSize());
	return true;
}


void CMemorySystem::MemorySystem_Shutdown()
{
	LOG_INFO("Memory System Shutdown");
}


void* CMemorySystem::MemAllocate(size_t numBytes, eMemAllocCategories eAllocCategory)
{
	if (eAllocCategory == eMemAllocCategories::MEM_CATEGORY_UNKNOWN)
	{
		LOG_WARN("MemorySystem->Alloc() - Category is 'UNKNOWN', re-class this allocation");
	}

	// TODO: Memory alignment
	void* block = nullptr;

	//block = Platform::MemoryAllocate(numBytes);

	block = m_InternalAllocator->Allocate(numBytes);

	if (block != nullptr)
	{
		m_numAllocations++;
		m_numBytesAllocated += numBytes;
		m_taggedAllocations[(size_t)eAllocCategory] += numBytes;

		return block;
	}

	// TODO: Handle these failures properly.
	LOG_ERROR("MemorySystem->MemAllocate could not allocate memory, returning nullptr.");
	return nullptr;
}


void CMemorySystem::MemFree(void* block, size_t size, eMemAllocCategories eAllocCategory)
{
	// TODO: Memory alignment

	m_numAllocations--;
	m_numBytesAllocated -= size;
	m_taggedAllocations[(size_t)eAllocCategory] -= size;
	
	// There is nothing wrong with calling free() on a nullptr,
	// so no need to check.
	m_InternalAllocator->Free(block);
}


void* CMemorySystem::MemReallocate(void* memory, size_t newSize)
{
	// TODO: Memory alignment
	void* block = nullptr;

	block = Platform::MemoryReallocate(memory, newSize);

	// Free the original block
	Platform::MemoryFree(memory);

	if (block != nullptr)
		return block;

	// TODO: Handle these failures properly.
	LOG_ERROR("MemorySystem->MemReallocate could not allocate memory, returning nullptr.");
	return nullptr;
}


void* CMemorySystem::MemCopy(void* dest, const void* source, size_t size)
{
	if (dest && source && size)
		return Platform::MemoryCopy(dest, source, size);

	LOG_ERROR("MemorySystem->MemCopy requires all parameters to be valid to operate. Returning nullptr.");
	return nullptr;
}


void* CMemorySystem::MemSet(void* memory, int value, size_t size)
{
	if (memory)
		return Platform::MemorySet(memory, value, size);

	LOG_ERROR("MemorySystem->MemSet requires a valid pointer to a memory block. Returning nullptr.");
	return nullptr;
}


void CMemorySystem::PrintMemoryStats()
{
	char memStatsString[5000] = "Memory Stats:\n";

	for (size_t i = 0; i < MAX_MEM_CATEGORIES; i++)
	{
		char categoryString[256]{ 0 };
		StringFormat_V(" %s\t: %d B\n", nullptr, categoryString, s_MemCategoryStrings[i], m_taggedAllocations[i]);
		StringConcat(memStatsString, categoryString);
	}

	char totalAllocationString[256]{ 0 };
	StringFormat_V(" Total Memory Allocations: %d", nullptr, totalAllocationString, m_numAllocations);
	StringConcat(memStatsString, totalAllocationString);

	LOG_INFO(memStatsString);
}


static CMemorySystem s_MemorySystem;
IMemorySystem* gpMemorySystem = &s_MemorySystem;

