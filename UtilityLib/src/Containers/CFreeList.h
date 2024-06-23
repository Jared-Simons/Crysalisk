//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2024
// 
// Purpose: A Free List to be used for dynamic memory allocation tracking.
// 
// ALL CREDIT FOR THE IMPLEMENTATION OF THIS STRUCTURE GOES TO THE KOHI GAME ENGINE!!!
// https://github.com/travisvroman/kohi/blob/main/engine/src/containers/freelist.c
//=============================================================================
#pragma once

#include <Core/Logging.h>

#include "CVector.h"


struct FreeListNode_t
{
	void* operator new(size_t size)
	{
		void* block = gpMemorySystem->MemAllocate(size, eMemAllocCategories::MEM_CATEGORY_LIST);
		return block;
	}

	void operator delete(void* ptr)
	{
		gpMemorySystem->MemFree(ptr, sizeof(FreeListNode_t), eMemAllocCategories::MEM_CATEGORY_LIST);
	}

	FreeListNode_t()
		: size{ 0 }
		, offset{ 0 }
		, pNext{ nullptr } 
	{}

	size_t			size;
	size_t			offset;
	FreeListNode_t* pNext;
};


class CFreeList
{
public:
	void* operator new(size_t size)
	{
		void* block = gpMemorySystem->MemAllocate(size, eMemAllocCategories::MEM_CATEGORY_LIST);
		return block;
	}

	void operator delete(void* ptr)
	{
		gpMemorySystem->MemFree(ptr, sizeof(CFreeList), eMemAllocCategories::MEM_CATEGORY_LIST);
	}

	CFreeList(size_t inTotalSize)
		: m_TotalSize{ inTotalSize }
	{
		// Calculate the number of nodes we can hold with the given size.
		m_MaxEntries = inTotalSize / sizeof(void*);

		// If the memory required is too small, we should warn about it.
		size_t minMem = (sizeof(CFreeList) + sizeof(FreeListNode_t)) * 8;
		if (inTotalSize < minMem)
		{
			LOG_WARN("Freelists are very inefficient with amounts of memory less then %iB: It is recommended to not use this structure in this case.", minMem);
		}

		m_pNodes = new FreeListNode_t[m_MaxEntries];
		m_pHead = &m_pNodes[0];

		m_pHead->offset = 0;
		m_pHead->size = inTotalSize;
		m_pHead->pNext = nullptr;
	}

	virtual ~CFreeList()
	{
		delete[] m_pNodes;
		memset(m_pNodes, 0, sizeof(FreeListNode_t) * m_MaxEntries);
		m_pNodes = nullptr;
	}

	bool Allocate(size_t size, size_t* outOffset)
	{
		FreeListNode_t* nodeToReturn = nullptr;

		FreeListNode_t* currNode = m_pHead;
		FreeListNode_t* prevNode = nullptr;

		while (currNode != nullptr)
		{
			if (currNode->size == size)
			{
				// Exact match. Just return the node.
				*outOffset = currNode->offset;
				if (prevNode)
				{
					prevNode->pNext = currNode->pNext;
					nodeToReturn = currNode;
				}
				else
				{
					// This node is the head of the list. Reassign the head
					// and return the previous head node.
					nodeToReturn = m_pHead;
					m_pHead = currNode->pNext;
				}
				ReturnNode(currNode);
				return true;
			}
			else if (currNode->size > size)
			{
				// Node is larger. Deduct the memory from it and move the offset
				// by that amount.
				*outOffset = currNode->offset;
				currNode->size -= size;
				currNode->offset += size;
				return true;
			}

			prevNode = currNode;
			currNode = currNode->pNext;
		}

		size_t remainingSpace = GetFreeSpace();
		LOG_WARN("CFreeList->Allocate() - no block with enough free space found (requested: %uB, available: %lluB).", m_TotalSize, remainingSpace);
		return false;
	}


	bool Free(size_t size, size_t offset)
	{
		FreeListNode_t* currNode = m_pHead;
		FreeListNode_t* prevNode = nullptr;

		while (currNode != nullptr)
		{
			if (currNode->offset == offset)
			{
				// Can just be appended to this node.
				currNode->size += size;

				// Check if this then connects the range between this and the next
				// node, and if so, combine them and return the second node...
				if (currNode->pNext && currNode->pNext->offset == currNode->offset + currNode->size)
				{
					currNode->size += currNode->pNext->size;
					FreeListNode_t* pNext = currNode->pNext;
					currNode->pNext = currNode->pNext->pNext;
					ReturnNode(pNext);
				}
				return true;
			}
			else if (currNode->offset > offset)
			{
				// Iterated beyond the space to be freed. Need a new node.
				FreeListNode_t* newNode = GetNode();
				newNode->offset = offset;
				newNode->size = size;

				// If there is a previous node, the new node should be inserted between this and it.
				if (prevNode)
				{
					prevNode->pNext = newNode;
					newNode->pNext = currNode;
				}
				else
				{
					// Otherwise, the new node becomes the head.
					newNode->pNext = currNode;
					m_pHead = newNode;
				}

				// Double-check next node to see if it can be joined.
				if (newNode->pNext && newNode->offset + newNode->size == newNode->pNext->offset)
				{
					newNode->size += newNode->pNext->size;
					FreeListNode_t* rubbish = newNode->pNext;
					newNode->pNext = rubbish->pNext;
					ReturnNode(rubbish);
				}

				// Double-check previous node to see if it can be joined.
				if (prevNode && prevNode->offset + prevNode->size == newNode->offset)
				{
					prevNode->size += newNode->size;
					FreeListNode_t* rubbish = newNode;
					prevNode->pNext = rubbish->pNext;
					ReturnNode(rubbish);
				}

				return true;
			}

			prevNode = currNode;
			currNode = currNode->pNext;
		}

		LOG_WARN("CFreeList->Free(): Unable to find block to be freed. Corruption possible?");
		return false;
	}

	FreeListNode_t* GetNode()
	{
		for (size_t i = 1; i < m_MaxEntries; i++)
		{
			if (m_pNodes[i].size == 0)
			{
				m_pNodes[i].pNext = nullptr;
				m_pNodes[i].offset = 0;
				return &m_pNodes[i];
			}
		}
	}

	// Small utility method to prepare a node to be pulled from the list
	void ReturnNode(FreeListNode_t* pNode)
	{
		pNode->offset = 0;
		pNode->size = 0;
		pNode->pNext = nullptr;
	}

	size_t GetFreeSpace() const
	{
		size_t runningTotal = 0;
		FreeListNode_t* currNode = m_pHead;
		
		while (currNode != nullptr)
		{
			runningTotal += currNode->size;
			currNode = currNode->pNext;
		}

		return runningTotal;
	}

	void Clear()
	{
		// Invalidate the offset for all but the first node. The invalid
		// value will be checked for when seeking a new node from the list.
		memset(m_pNodes, 0, sizeof(FreeListNode_t) * m_MaxEntries);

		// Reset the head to occupy the entire list.
		m_pHead->offset = 0;
		m_pHead->size = m_TotalSize;
		m_pHead->pNext = nullptr;
	}

	size_t GetNumMaxEntries() const { return m_MaxEntries; }
	size_t GetTotalSize() const { return m_TotalSize; }

private:
	size_t			m_TotalSize;
	size_t			m_MaxEntries;
	FreeListNode_t* m_pHead;
	FreeListNode_t* m_pNodes;
};


