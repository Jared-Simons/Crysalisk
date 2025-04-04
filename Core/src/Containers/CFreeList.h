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
		CFreeList(size_t bytesToTrack)
		{
			m_iNumBytesTracked = bytesToTrack;
			m_iNumFreelistNodes = m_iNumBytesTracked / sizeof(FreeListNode_t);
		}

		~CFreeList()
		{

		}

		// Custom placement delete
		// TODO: IMPLEMENT!

		size_t	GetInternalMemoryRequirement() { return sizeof(FreeListNode_t) * m_iNumFreelistNodes; }
		void	Init(void* pMemory);

		size_t	Allocate(size_t size);
		void	Free(size_t size, size_t offset);

	private:

		FreeListNode_t* GetNewNode();
		void			ReleaseNodeBackToFreelist(FreeListNode_t* pNode);

	private:
		FreeListNode_t* m_pNodeArray{ nullptr };

		FreeListNode_t* m_HeadNode{ nullptr };
		FreeListNode_t* m_pNextFreeNode{ m_HeadNode };
		size_t			m_iNumBytesTracked{ 0 };
		size_t			m_iNumFreelistNodes{ 0 };
	};

	void CFreeList::Init(void* pMemory)
	{
		CAssert(pMemory);

		m_pNodeArray = new (pMemory) FreeListNode_t[m_iNumFreelistNodes];

		// Set the head node.
		m_HeadNode = &m_pNodeArray[0];

		m_HeadNode->Size = m_iNumBytesTracked;
		m_HeadNode->Offset = 0;
		m_HeadNode->pNextNode = nullptr;
	}

	size_t CFreeList::Allocate(size_t size)
	{
		// Make sure we don't try to allocate more than we have available.
		CAssert(size <= m_iNumBytesTracked);

		FreeListNode_t* pPreviousNode = nullptr;
		FreeListNode_t* pCurrentNode = m_HeadNode;
		int retDataOffset = 0;

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
			}

			pPreviousNode = pCurrentNode;
			pCurrentNode = pCurrentNode->pNextNode;
		}

		return retDataOffset;
	}

	void CFreeList::Free(size_t size, size_t offset)
	{
		FreeListNode_t* pPreviousNode = nullptr;
		FreeListNode_t* pCurrentNode = m_HeadNode;

		if (m_HeadNode->Offset > offset)
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

				// Check the next node to see if it's contiguous with the new combined node.
				if (pPreviousNode->Offset + pPreviousNode->Size == pCurrentNode->Offset)
				{
					// We're contiguous, time to join!
					pPreviousNode->Size += pCurrentNode->Size;
					pPreviousNode->pNextNode = nullptr;

					// Release the node back to the list of empty nodes!
					ReleaseNodeBackToFreelist(pCurrentNode);
				}
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
	}

	FreeListNode_t* CFreeList::GetNewNode()
	{
		for (int idx = 0; idx < m_iNumFreelistNodes; idx++)
		{
			if (m_pNodeArray[idx].Size == 0)	// If the node has a size of zero, then it hasn't been touched yet.
			{
				return &m_pNodeArray[idx];	
			}
		}

		CAssert(0); // TODO: HANDLE THIS
	}

	void CFreeList::ReleaseNodeBackToFreelist(FreeListNode_t* pNode)
	{
		CAssert(pNode);
		pNode->Offset = pNode->Size = 0;
		pNode->pNextNode = nullptr;
	}
}