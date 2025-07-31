//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: Re-routes global new and deletes to our own memory system.
//=============================================================================
#include <Memory/MemoryManager.h>

//-------------------------------------------------------------------------
// Purpose: Allocates 'size' bytes of memory.
// Input:	size - The number of bytes to allocate.
// Output:	A pointer to the new memory block.
//-------------------------------------------------------------------------
void* operator new(size_t size)
{
	void* pMemBlock = g_pMemoryManager->Allocate(size);
	return pMemBlock;
}

//-------------------------------------------------------------------------
// Purpose: Frees a block of memory.
// Input:	pMemBlock - The block of memory to be freed.
// Output:	none.
//-------------------------------------------------------------------------
void operator delete(void* pMemBlock)
{
	if (pMemBlock && g_pMemoryManager->IsInitialized())
		g_pMemoryManager->Free(pMemBlock);
}

//-------------------------------------------------------------------------
// Purpose: Allocates 'size' bytes of memory.
// Input:	size - The number of bytes to allocate.
// Output:	A pointer to the new memory block.
//-------------------------------------------------------------------------
void* operator new[](size_t size)
{
	void* pMemBlock = g_pMemoryManager->Allocate(size);
	return pMemBlock;
}

//-------------------------------------------------------------------------
// Purpose: Frees a block of memory.
// Input:	pMemBlock - The block of memory to be freed.
// Output:	none.
//-------------------------------------------------------------------------
void operator delete[](void* pMemBlock)
{
	if (pMemBlock && g_pMemoryManager->IsInitialized())
		g_pMemoryManager->Free(pMemBlock);
}