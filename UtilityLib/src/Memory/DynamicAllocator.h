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
	DynamicAllocator(size_t sizeInBytes);
	virtual ~DynamicAllocator();

	void* Allocate(size_t size, size_t alignment = 2);
	void Free(void* pMemBlock);

	size_t GetAllocatorSize() const { return m_Size; }
	size_t GetBytesAllocated() const { return m_Allocated; }
	size_t GetFreeSpace() const { return m_Size - m_Allocated; }

	void ReportMemoryFragmentation();

private:
	DynamicAllocator() = delete;

private:
	size_t			m_Size;
	size_t			m_Allocated;
	void*			m_pData;
	FreeListNode_t* m_pHeadNode;
};

DynamicAllocator::DynamicAllocator(size_t sizeInBytes)
	: m_Size{ sizeInBytes }
	, m_Allocated{ 0 }
	, m_pData{ nullptr }
	, m_pHeadNode{ nullptr }
{
	m_pData = Platform::MemoryAllocate(sizeInBytes);
	
	FreeListNode_t* pFirstNode = static_cast<FreeListNode_t*>(m_pData);
	pFirstNode->blockSize = sizeInBytes;
	pFirstNode->offset = 0;
	pFirstNode->alignment = 0;
	pFirstNode->pNext = nullptr;
	m_pHeadNode = pFirstNode;

	LOG_INFO("Dynamic Allocator created with %d bytes.", m_Size);
}

DynamicAllocator::~DynamicAllocator()
{
	LOG_INFO("Destroying Allocator with %d bytes.", m_Size);

	memset(m_pData, 0, m_Size);
	Platform::MemoryFree(m_pData);
	m_pData = nullptr;
}

void* DynamicAllocator::Allocate(size_t size, size_t alignment)
{
	if (!IsNumberAPowerOfTwo(alignment))
	{
		LOG_WARN("DynamicAllocator->Allocate() - Alignment MUST be a power of two. Memory might be unaligned.");
	}

	FreeListNode_t* pPreviousNode = nullptr;
	FreeListNode_t* pCurrentNode = m_pHeadNode;
	FreeListNode_t* pNextNode = nullptr;

	while (pCurrentNode != nullptr)
	{
		if (pCurrentNode->blockSize >= size)
		{
			// Found a block with at least enough space. This node will do.
			break;
		}

		pPreviousNode = pCurrentNode;
		pCurrentNode = pCurrentNode->pNext;
	}

	if (pCurrentNode == nullptr)
	{
		LOG_ERROR("DynamicAllocator->Allocate() - Failed to find free block of memory! Returning nullptr.");
		return nullptr;
	}

	m_Allocated += size + ALLOC_HEADER_SIZE;

	pNextNode = pCurrentNode->pNext;

	if (pPreviousNode == nullptr)
	{
		// If there was no previous node in the list, then we must be the current head node.
		pCurrentNode->blockSize = size;
		pCurrentNode->alignment = alignment;
		pCurrentNode->pNext = nullptr;

		// We still have enough room to add a new node after
		if (m_Allocated + ALLOC_HEADER_SIZE < m_Size)
		{
			if (pNextNode == nullptr)
			{
				// Need to create a new node here
				void* pNewData = static_cast<char*>(m_pData) + (size + ALLOC_HEADER_SIZE);
				pNextNode = static_cast<FreeListNode_t*>(pNewData);

				pNextNode->blockSize = m_Size - m_Allocated;
				pNextNode->offset = m_Allocated;
				pNextNode->alignment = 2;
				pNextNode->pNext = nullptr;
			}
		}

		m_pHeadNode = pNextNode;
	}
	else
	{
		pCurrentNode->blockSize = size;
		pCurrentNode->alignment = alignment;
		pCurrentNode->pNext = nullptr;

		if (pNextNode == nullptr)
		{
			// Need to create a new node here
			void* pNewData = static_cast<char*>(m_pData) + (size + ALLOC_HEADER_SIZE);
			pNextNode = static_cast<FreeListNode_t*>(pNewData);

			pNextNode->blockSize = m_Size - m_Allocated;
			pNextNode->offset = pCurrentNode->offset + pCurrentNode->blockSize;
			pNextNode->alignment = 2;
			pNextNode->pNext = nullptr;
		}

		// Remove the current node from the list
		pPreviousNode->pNext = pNextNode;
	}

	//void* pMemBlock = pCurrentNode + ALLOC_HEADER_SIZE;
	void* pMemBlock = static_cast<char*>((void*)pCurrentNode) + ALLOC_HEADER_SIZE;
	memset(pMemBlock, 0, size);

	return pMemBlock;
}

void DynamicAllocator::Free(void* pMemBlock)
{
	if (!pMemBlock)
	{
		LOG_WARN("DynamicAllocator->Free() - Cannot free null memory!");
		return;
	}

	void* pNodeAddress = static_cast<char*>(pMemBlock) - ALLOC_HEADER_SIZE;
	FreeListNode_t* pNodeToReturn = static_cast<FreeListNode_t*>(pNodeAddress);

	// Look at the node to determine where the memory should be returned to.
	if (pNodeToReturn->offset == 0)
	{
		// This is the head of the allocator's memory block.
		// We will become the new head node.
		FreeListNode_t* pCurrentHeadNode = m_pHeadNode;

		// Clear out whatever was in this memory block.
		memset(pMemBlock, 0, pNodeToReturn->blockSize);
		
		pNodeToReturn->pNext = pCurrentHeadNode;

		m_pHeadNode = pNodeToReturn;
	}
	else
	{
		FreeListNode_t* pPreviousNode = m_pHeadNode;
		FreeListNode_t* pCurrentNode = m_pHeadNode->pNext;

		while (pCurrentNode != nullptr)
		{
			if ((pPreviousNode->offset < pNodeToReturn->offset) && (pCurrentNode->offset > pNodeToReturn->offset))
			{
				// We found a suitable place to link our node back.
				memset(pMemBlock, 0, pNodeToReturn->blockSize);

				pPreviousNode->pNext = pNodeToReturn;
				pNodeToReturn->pNext = pCurrentNode;
			}

			pPreviousNode = pCurrentNode;
			pCurrentNode = pCurrentNode->pNext;
		}
	}

	m_Allocated -= pNodeToReturn->blockSize + ALLOC_HEADER_SIZE;

	ReportMemoryFragmentation();

	// TODO: Clean up memory fragmentation!
}

void DynamicAllocator::ReportMemoryFragmentation()
{
	int numberOfFreeMemoryNodes = 0;
	int numberOfContiguousNodes = 0;
	FreeListNode_t* pCurrentNode = m_pHeadNode;

	while (pCurrentNode != nullptr)
	{
		numberOfFreeMemoryNodes += 1;

		if (pCurrentNode->pNext)
		{
			size_t size1 = ALLOC_HEADER_SIZE + pCurrentNode->offset + pCurrentNode->blockSize;
			size_t size2 = pCurrentNode->pNext->offset;

			if ((ALLOC_HEADER_SIZE + pCurrentNode->offset + pCurrentNode->blockSize) == pCurrentNode->pNext->offset)
				numberOfContiguousNodes += 2;
		}

		pCurrentNode = pCurrentNode->pNext;
	}

	LOG_DEBUG("Dynamic Allocator Memory Fragmentation Report: %d Free Node(s). (%d contiguous node(s))", numberOfFreeMemoryNodes, numberOfContiguousNodes);
}

struct MyTestStruct
{
	size_t a;
	int b;
	bool c;
	char* d;
};

