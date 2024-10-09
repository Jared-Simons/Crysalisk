//=============================================================================
// Author: Jared Simons
// Date Created: 7/7/2024
// 
// Purpose: A Dynamic Memory Allocator.
//=============================================================================

#include "DynamicAllocator.h"

DynamicAllocator::DynamicAllocator(size_t sizeInBytes, void* pMemBlock)
	: m_Size{ sizeInBytes }
	, m_Allocated{ 0 }
	, m_pData{ pMemBlock }
	, m_pHeadNode{ nullptr }
{
	if (!pMemBlock)
	{
		LOG_ERROR("Dynamic Allocator REQUIRES a pointer to an allocated block of memory. Creation failed.");
		return;
	}

	FreeListNode_t* pFirstNode = static_cast<FreeListNode_t*>(m_pData);
	pFirstNode->blockSize = sizeInBytes;
	pFirstNode->offset = 0;
	pFirstNode->alignment = 0;
	pFirstNode->pNext = nullptr;
	m_pHeadNode = pFirstNode;

	//LOG_INFO("Dynamic Allocator created with %d bytes.", m_Size);
}

DynamicAllocator::~DynamicAllocator()
{
	//LOG_INFO("Destroying Allocator with %d bytes.", m_Size);
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
				void* pNewData = static_cast<char*>(m_pData) + (pCurrentNode->offset + size + ALLOC_HEADER_SIZE);
				pNextNode = static_cast<FreeListNode_t*>(pNewData);

				pNextNode->offset = pCurrentNode->offset + size + ALLOC_HEADER_SIZE;
				pNextNode->blockSize = m_Size - pNextNode->offset;
				pNextNode->alignment = 2;
				pNextNode->pNext = nullptr;
			}
		}
		else
		{
			LOG_WARN("Dynamic Allocator -> We are full!");
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
			void* pNewData = static_cast<char*>(m_pData) + (pCurrentNode->offset + size + ALLOC_HEADER_SIZE);
			pNextNode = static_cast<FreeListNode_t*>(pNewData);

			pNextNode->offset = pCurrentNode->offset + size + ALLOC_HEADER_SIZE;
			pNextNode->blockSize = m_Size - pNextNode->offset;
			pNextNode->alignment = 2;
			pNextNode->pNext = nullptr;
		}

		// Remove the current node from the list
		pPreviousNode->pNext = pNextNode;
	}

	//void* pMemBlock = pCurrentNode + ALLOC_HEADER_SIZE;
	void* pMemBlock = static_cast<char*>((void*)pCurrentNode) + ALLOC_HEADER_SIZE;
	Platform::MemorySet(pMemBlock, 0, size);

#ifdef CRYS_DEBUG
	//LOG_VERBOSE("Memory Allocation -> size %d bytes, at address 0x%p.", pCurrentNode->blockSize, pMemBlock);
#endif

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
	if (m_pHeadNode == nullptr)
	{
		// If there is no head node, then the whole memory block must be allocated.
		// Set this to be the head node and zero it out.
		Platform::MemorySet(pMemBlock, 0, pNodeToReturn->blockSize);

		m_pHeadNode = pNodeToReturn;
	}
	else
	{
		if (pNodeToReturn->offset == 0)
		{
			// This is the head of the allocator's memory block.
			// We will become the new head node.
			FreeListNode_t* pCurrentHeadNode = m_pHeadNode;

			// Clear out whatever was in this memory block.
			Platform::MemorySet(pMemBlock, 0, pNodeToReturn->blockSize);

			pNodeToReturn->pNext = pCurrentHeadNode;

			m_pHeadNode = pNodeToReturn;
		}
		else
		{
			FreeListNode_t* pPreviousNode = m_pHeadNode;
			FreeListNode_t* pCurrentNode = m_pHeadNode->pNext;

			if (pCurrentNode == nullptr)
			{
				// If there is only one head node (the current head node), figure out where to append this node.
				if (pPreviousNode->offset < pNodeToReturn->offset)
				{
					Platform::MemorySet(pMemBlock, 0, pNodeToReturn->blockSize);
					pPreviousNode->pNext = pNodeToReturn;
				}
				else
				{
					Platform::MemorySet(pMemBlock, 0, pNodeToReturn->blockSize);
					m_pHeadNode = pNodeToReturn;
					pNodeToReturn->pNext = pPreviousNode;
				}
			}
			else
			{
				while (pCurrentNode != nullptr)
				{
					if ((pPreviousNode->offset < pNodeToReturn->offset) && (pCurrentNode->offset > pNodeToReturn->offset))
					{
						// We found a suitable place to link our node back.
						Platform::MemorySet(pMemBlock, 0, pNodeToReturn->blockSize);

						pPreviousNode->pNext = pNodeToReturn;
						pNodeToReturn->pNext = pCurrentNode;
					}

					if (pCurrentNode->blockSize == 0 && pCurrentNode->offset == 0 && pCurrentNode->alignment == 0)
					{
						// We have encountered an invalid node...
						// If we reach here, just append our node to the previous node and warn about it
						// This ideally should never happen...
						LOG_WARN("DynamicAllocator->Free() - Invalid node detected!");

						pPreviousNode->pNext = pNodeToReturn;
						pNodeToReturn->pNext = nullptr;
						break;
					}

					pPreviousNode = pCurrentNode;
					pCurrentNode = pCurrentNode->pNext;
				}
			}
		}
	}

	m_Allocated -= pNodeToReturn->blockSize + ALLOC_HEADER_SIZE;

#ifdef CRYS_DEBUG
	//LOG_VERBOSE("Memory Freed -> size %d bytes, at address 0x%p.", pNodeToReturn->blockSize, pMemBlock);
#endif

	//ReportMemoryFragmentation();

	CleanupMemoryFragmentation();
}

void DynamicAllocator::ReportMemoryFragmentation()
{
	i32 numberOfFreeMemoryNodes = 0;
	i32 numberOfContiguousNodes = 0;
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

void DynamicAllocator::CleanupMemoryFragmentation()
{
	FreeListNode_t* pCurrentNode = m_pHeadNode;

	while (pCurrentNode)
	{
		if (pCurrentNode->pNext)
		{
			if ((pCurrentNode->offset + ALLOC_HEADER_SIZE + pCurrentNode->blockSize) == pCurrentNode->pNext->offset)
			{
#ifdef CRYS_DEBUG
				//LOG_VERBOSE("Cleaning up memory fragmentation");
#endif

				pCurrentNode->blockSize = pCurrentNode->blockSize + ALLOC_HEADER_SIZE + pCurrentNode->pNext->blockSize;
				pCurrentNode->pNext = pCurrentNode->pNext->pNext;
			}
		}

		pCurrentNode = pCurrentNode->pNext;
	}
}
