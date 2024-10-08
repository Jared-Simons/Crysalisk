//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: A Custom Linked-List.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Core/Logging.h>

// Forward Declarations
template<typename T>
class CList;
template<typename T>
class CListNode;

template<typename T>
class CListNode
{
public:
	CListNode()
		: m_pNext{ nullptr }
		, m_pData{} {}

	CListNode(T val)
		: m_pNext{ nullptr }
		, m_pData{ val } {}

	CListNode(CListNode& otherNode)
		: m_pNext{ otherNode.m_pNext }
		, m_pData{ otherNode.m_pData } {}

	CListNode* m_pNext;
	T m_pData;
};

template<typename T>
class CList
{
public:

	virtual ~CList()
	{
		while (m_Size > 1)
		{
			PopBack();
		}

		if (m_Size == 1)
		{
			delete m_pHead;
			m_Size--;
		}

		//LOG_DEBUG("CList Destroyed - Size: %d", m_Size);
	}

	//==============================================
	// BIG WALL OF CONSTRUCTORS
	//==============================================

	CList()
		: m_Size{ 0 }
		, m_pHead{ nullptr } {}

	CList(T element)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;
		PushBack(element);
	}

	CList(T element1, T element2)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2 };
		BuildListFromVariableConstructor(2, elements);
	}

	CList(T element1, T element2, T element3)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3 };
		BuildListFromVariableConstructor(3, elements);
	}

	CList(T element1, T element2, T element3, T element4)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4 };
		BuildListFromVariableConstructor(4, elements);
	}

	CList(T element1, T element2, T element3, T element4, T element5)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5 };
		BuildListFromVariableConstructor(5, elements);
	}

	CList(T element1, T element2, T element3, T element4, T element5, T element6)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6 };
		BuildListFromVariableConstructor(6, elements);
	}

	CList(T element1, T element2, T element3, T element4, T element5, T element6, T element7)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7 };
		BuildListFromVariableConstructor(7, elements);
	}

	CList(T element1, T element2, T element3, T element4, T element5, T element6, T element7, T element8)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7, element8 };
		BuildListFromVariableConstructor(8, elements);
	}

	CList(T element1, T element2, T element3, T element4, T element5, T element6, T element7, T element8, T element9)
	{
		m_pHead = new CListNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7, element8, element9 };
		BuildListFromVariableConstructor(9, elements);
	}

private:
	void BuildListFromVariableConstructor(size_t numOfElements, T values[])
	{
		for (i32 i = 0; i < numOfElements; i++)
		{
			PushBack(values[i]);
		}
	}

public:

	T& operator[](i32 index)
	{
		CListNode<T>* currentNode = m_pHead;
		for (i32 i = 0; i < index; i++)
			currentNode = currentNode->m_pNext;

		return currentNode->m_pData;
	}

	T Front()
	{
		if (m_pHead)
			return m_pHead->m_pData;

		LOG_ERROR("CList->Front() - Attempting to access data that does not exist, returning -1");
		return -1;
	}

	T Back()
	{
		CListNode<T>* currentNode = m_pHead;
		while (currentNode->m_pNext != nullptr)
			currentNode = currentNode->m_pNext;

		return currentNode->m_pData;
	}

	void PushFront(T val)
	{
		CListNode<T>* newValue = new CListNode(val);
		newValue->m_pNext = m_pHead;
		m_pHead = newValue;
		m_Size++;
	}
	
	void PushBack(T val)
	{
		CListNode<T>* currentPtr = m_pHead;
		if (m_Size == 0)
		{
			m_pHead = new CListNode(val);
			m_Size++;
			return;
		}

		while (currentPtr->m_pNext != nullptr)
			currentPtr = currentPtr->m_pNext;

		currentPtr->m_pNext = new CListNode(val);
		m_Size++;
	}

	T PopFront()
	{
		CListNode<T>* poppedNode = m_pHead;

		m_pHead = poppedNode->m_pNext;
		T poppedValue = poppedNode->m_pData;
		delete(poppedNode);

		m_Size--;
		return poppedValue;
	}

	T PopBack()
	{
		CListNode<T>* newLastNode = m_pHead;

		// Get the second to last node in the last. (more than one element must exist to chain these pointers)
		while (m_Size != 1 && newLastNode->m_pNext->m_pNext != nullptr)
			newLastNode = newLastNode->m_pNext;

		T poppedValue = newLastNode->m_pNext->m_pData;

		delete newLastNode->m_pNext;
		newLastNode->m_pNext = nullptr;
		m_Size--;

		return poppedValue;
	}

	T& Insert(size_t newPos, T element, size_t numElements = 1)
	{
		// Get a pointer to the node at the one prior the given index
		CListNode<T>* newPosIndexSub1 = m_pHead;

		// Lists are one-indexed it appears
		size_t index = 1;
		while (index != (--newPos))
			newPosIndexSub1 = newPosIndexSub1->m_pNext;

		// Save off a copy to the next element for later
		CListNode<T>* linkToBeAdjusted = new CListNode<T>(*newPosIndexSub1->m_pNext);

		// Insert a new node at the element's position
		newPosIndexSub1->m_pNext = new CListNode<T>(element);

		// Insert more elements if provided
		size_t elementsAdded = 1;
		CListNode<T>* newElement = newPosIndexSub1->m_pNext;
		while (elementsAdded != numElements)
		{
			CListNode<T>* newNode = new CListNode<T>;
			newElement->m_pNext = newNode;
			newNode->m_pData = element;
			newElement = newNode;
			m_Size++;
			elementsAdded++;
		}
		
		// Link up the adjusted nodes here
		newElement->m_pNext = linkToBeAdjusted;

		m_Size++;
		return newPosIndexSub1->m_pNext->m_pData;
	}

	size_t GetSize() const { return m_Size; }

private:
	/* The current number of elements in the list (not necessarily the capacity) */
	size_t m_Size;

	/* The head node of the linked list. */
	CListNode<T>* m_pHead;
};

