//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2024
// 
// Purpose: A Dynamic Memory Allocator.
//=============================================================================
#pragma once

#include <Core/Logging.h>
#include <Platform/Platform.h>

struct FreeListNode_t
{
	size_t					blockSize;
	size_t					offset;
	size_t					alignment;
	struct FreeListNode_t*	pNext;
};

const size_t ALLOC_HEADER_SIZE = sizeof(FreeListNode_t);

class DynamicAllocator
{
public:
	DynamicAllocator(size_t sizeInBytes, void* pMemBlock);
	virtual ~DynamicAllocator();

	void* Allocate(size_t size, size_t alignment = 2);
	void Free(void* pMemBlock);

	size_t GetAllocatorSize() const { return m_Size; }
	size_t GetBytesAllocated() const { return m_Allocated; }
	size_t GetFreeSpace() const { return m_Size - m_Allocated; }
	void* GetMemoryBlock() const { return m_pData; }

	void ReportMemoryFragmentation();
	void CleanupMemoryFragmentation();

private:
	DynamicAllocator() = delete;

private:
	size_t			m_Size;
	size_t			m_Allocated;
	void*			m_pData;
	FreeListNode_t* m_pHeadNode;
};
