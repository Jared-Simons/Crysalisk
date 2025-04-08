#include "Memory/MemoryManager.h"

void* operator new(size_t size)
{
	void* pMemBlock = g_pMemoryManager->Allocate(size);
	return pMemBlock;
}

void operator delete(void* pMemBlock)
{
	if (pMemBlock)
		g_pMemoryManager->Free(pMemBlock);
}

void* operator new[](size_t size)
{
	void* pMemBlock = g_pMemoryManager->Allocate(size);
	return pMemBlock;
}

void operator delete[](void* pMemBlock)
{
	if (pMemBlock)
		g_pMemoryManager->Free(pMemBlock);
}