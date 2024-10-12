//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2024
// 
// Purpose: A Dynamic Memory Allocator.
//=============================================================================
#pragma once

#include <Defines.h>
#include "IMemAllocator.h"

#include <Core/Logging.h>
#include <Platform/Platform.h>

struct FreeListNode_t
{
	u64					blockSize;
	u64					offset;
	u64					alignment;
	struct FreeListNode_t*	pNext;
};

const u64 ALLOC_HEADER_SIZE = sizeof(FreeListNode_t);

class DynamicAllocator : public IMemAllocator
{
public:
	DynamicAllocator(u64 sizeInBytes, void* pMemBlock);
	virtual ~DynamicAllocator();

	void* Allocate(u64 size, u64 alignment = 2) override;
	void Free(void* pMemBlock) override;

	u64 GetAllocatorSize() const { return m_Size; }
	u64 GetBytesAllocated() const { return m_Allocated; }
	u64 GetFreeSpace() const { return m_Size - m_Allocated; }
	void* GetMemoryBlock() const { return m_pData; }

	void ReportMemoryFragmentation();
	void CleanupMemoryFragmentation();

private:
	DynamicAllocator() = delete;

private:
	u64			m_Size;
	u64			m_Allocated;
	void*			m_pData;
	FreeListNode_t* m_pHeadNode;
};
