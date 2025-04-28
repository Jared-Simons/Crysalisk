//=============================================================================
// Author: Jared Simons
// Date Created: 4/2/2025
// 
// Purpose: Freelist used to track ranges of memory.
//=============================================================================
#pragma once

#include "Core/Assertions.h"

#include <new> // for placement new

namespace Utl
{
	struct FreeListNode_t
	{
		size_t			Size{ 0 };
		size_t			Offset{ 0 };
		FreeListNode_t* pNextNode{ nullptr };
	};

	class CFreeList
	{
	public:
		CFreeList() {}

		~CFreeList()
		{

		}

		// Custom placement delete
		// TODO: IMPLEMENT!

		void	SetNumBytesToTrack(size_t bytesToTrack) { m_iNumBytesTracked = bytesToTrack; m_iNumFreelistNodes = bytesToTrack / (sizeof(void*) * sizeof(FreeListNode_t)); }
		size_t	GetInternalMemoryRequirement() const { return m_iNumFreelistNodes; }
		void	InitFreelist(void* pMemory)
		{
			CAssert(pMemory);

			m_pNodeArray = new (pMemory) FreeListNode_t[m_iNumFreelistNodes];

			// Set the head node.
			m_HeadNode = &m_pNodeArray[0];

			m_HeadNode->Size = m_iNumBytesTracked;
			m_HeadNode->Offset = 0;
			m_HeadNode->pNextNode = nullptr;
		}

		size_t	Allocate(size_t size)
		{
			// Make sure we don't try to allocate more than we have available.
			CAssert(size <= m_iNumBytesTracked);

			FreeListNode_t* pPreviousNode = nullptr;
			FreeListNode_t* pCurrentNode = m_HeadNode;
			size_t retDataOffset = 0;

			// Search the freelist for an available block of memory.
			while (pCurrentNode != nullptr)
			{
				if (pCurrentNode->Size >= size)
				{
					// This node is large enough to support our allocation!
					if (pCurrentNode->Size - size > 0)
					{
						retDataOffset = pCurrentNode->Offset;

						// There will be some memory leftover from this allocation, shift this node over!
						pCurrentNode->Size -= size;
						pCurrentNode->Offset += size;
						break;
					}
					else
					{
						// This node is just large enough to hold the data with none leftover.
						// Just return the node to the list.
						retDataOffset = pCurrentNode->Offset;

						if (m_HeadNode == pCurrentNode)
						{
							// We need to set the head node to the next one.
							m_HeadNode = pCurrentNode->pNextNode;
						}
						else
						{
							// Attach the next node to the previous one.
							pPreviousNode->pNextNode = pCurrentNode->pNextNode;
						}
						ReleaseNodeBackToFreelist(pCurrentNode);
						CAssert(retDataOffset != 0);
						return retDataOffset; // Early return here since pCurrentNode will be nullified.
					}
				}

				pPreviousNode = pCurrentNode;
				pCurrentNode = pCurrentNode->pNextNode;
			}

			return retDataOffset;
		}
		void	Free(size_t size, size_t offset)
		{
			FreeListNode_t* pPreviousNode = nullptr;
			FreeListNode_t* pCurrentNode = m_HeadNode;

			if (m_HeadNode->Offset >= offset)	// >= because the offset could just be 0, if we're freeing the first range of the memory block.
			{
				// This node comes before the current headnode in the data block.
				// Make this the new head node!
				if (offset + size == m_HeadNode->Offset)
				{
					// We don't need to create a whole new node... these blocks are contiguous so they can just be appended!
					m_HeadNode->Offset = offset;
					m_HeadNode->Size += size;
				}
				else
				{
					// These are not contiguous blocks... we need a new node here.
					FreeListNode_t* pNewNode = GetNewNode();
					CAssert(pNewNode);

					pNewNode->Offset = offset;
					pNewNode->Size = size;
					pNewNode->pNextNode = m_HeadNode;

					// Due to the above if check, we know that this block of memory comes before the current head node.
					// Assign it as such.
					m_HeadNode = pNewNode;
				}
			}
			else
			{
				// This node comes after the current headnode in the data block.
				// Append it to the closest node we can find.
				while (pCurrentNode && pCurrentNode->Offset < offset)
				{
					pPreviousNode = pCurrentNode;
					pCurrentNode = pCurrentNode->pNextNode;
				}

				// At this point, the previous node should be the closest free range of memory to us, so append ourselves there!
				CAssert(pPreviousNode);

				if (pPreviousNode->Offset + pPreviousNode->Size == offset)
				{
					// The blocks of memory are contiguous! No need for a new node here.
					pPreviousNode->Size += size;

					// No early return here because we might need to compact our node list.
				}
				else if (pCurrentNode == nullptr)
				{
					LOG_ERROR("CFreeList::Free -> Unable to free memory block with offset %d and size %d, memory leak is likely.", offset, size);
					return;
				}
				else if (pCurrentNode == nullptr && pPreviousNode->Offset + size < offset)
				{
					// This block comes after the current free ranges, but not directly after.
					// There needs to be a new node appended at the very end of the freelist!
					FreeListNode_t* pNewNode = GetNewNode();
					CAssert(pNewNode);
					pPreviousNode->pNextNode = pNewNode;
					pNewNode->Offset = offset;
					pNewNode->Size = size;

					// Early return is okay, there should be nothing new to compact here.
					return;
				}
				else if (offset + size == pCurrentNode->Offset)
				{
					// In this case, the current range of memory is contiguous with the node after it, combine them into one node!
					pCurrentNode->Size += size;
					pCurrentNode->Offset = offset;
				}
				else
				{
					// The blocks of memory are not contiguous! We need a new node here.
					FreeListNode_t* pNewNode = GetNewNode();
					CAssert(pNewNode);

					pNewNode->Offset = offset;
					pNewNode->Size = size;
					pNewNode->pNextNode = pCurrentNode;
					pPreviousNode->pNextNode = pNewNode;
					return;
				}
			}

			// Defragment the freelist here
			while (pPreviousNode && pCurrentNode &&
				pPreviousNode->Offset + pPreviousNode->Size == pCurrentNode->Offset)
			{
				//LOG_INFO("NEED TO COMPACT!");
				pPreviousNode->Size += pCurrentNode->Size;
				pPreviousNode->pNextNode = pCurrentNode->pNextNode;
				ReleaseNodeBackToFreelist(pCurrentNode);
			}
		}

	private:

		FreeListNode_t* GetNewNode()
		{
			for (int idx = 0; idx < m_iNumFreelistNodes; idx++)
			{
				if (m_pNodeArray[idx].Size == 0)	// If the node has a size of zero, then it hasn't been touched yet.
				{
					return &m_pNodeArray[idx];
				}
			}

			CAssert(0); // TODO: HANDLE THIS
			return nullptr;
		}

		void ReleaseNodeBackToFreelist(FreeListNode_t* pNode)
		{
			CAssert(pNode);
			pNode->Offset = pNode->Size = 0;
			pNode->pNextNode = nullptr;
		}

	private:
		FreeListNode_t* m_pNodeArray{ nullptr };

		FreeListNode_t* m_HeadNode{ nullptr };
		FreeListNode_t* m_pNextFreeNode{ m_HeadNode };
		size_t			m_iNumBytesTracked{ 0 };
		size_t			m_iNumFreelistNodes{ 0 };
	};
}