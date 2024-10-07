//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: A Custom Set Type.
//=============================================================================
#pragma once

#include <Defines.h>

// CSets are essentially CSets with extra checks for repetitive entries.
// Much of this is similar to CSet's layout
template<typename T>
class CSetNode
{
public:
	CSetNode()
		: m_pNext{ nullptr }
		, m_pData{} {}

	CSetNode(T val)
		: m_pNext{ nullptr }
		, m_pData{ val } {}

	CSetNode(CSetNode& otherNode)
		: m_pNext{ otherNode.m_pNext }
		, m_pData{ otherNode.m_pData } {}

	CSetNode* m_pNext;
	T m_pData;
};

template<typename T>
class CSet
{
public:

	virtual ~CSet()
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

		//LOG_DEBUG("CSet Destroyed - Size: %d", m_Size);
	}

	//==============================================
	// BIG WALL OF CONSTRUCTORS
	//==============================================

	CSet()
		: m_Size{ 0 }
		, m_pHead{ nullptr } {}

	CSet(T element)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;
		PushBack(element);
	}

	CSet(T element1, T element2)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2 };
		BuildListFromVariableConstructor(2, elements);
	}

	CSet(T element1, T element2, T element3)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3 };
		BuildListFromVariableConstructor(3, elements);
	}

	CSet(T element1, T element2, T element3, T element4)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4 };
		BuildListFromVariableConstructor(4, elements);
	}

	CSet(T element1, T element2, T element3, T element4, T element5)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5 };
		BuildListFromVariableConstructor(5, elements);
	}

	CSet(T element1, T element2, T element3, T element4, T element5, T element6)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6 };
		BuildListFromVariableConstructor(6, elements);
	}

	CSet(T element1, T element2, T element3, T element4, T element5, T element6, T element7)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7 };
		BuildListFromVariableConstructor(7, elements);
	}

	CSet(T element1, T element2, T element3, T element4, T element5, T element6, T element7, T element8)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7, element8 };
		BuildListFromVariableConstructor(8, elements);
	}

	CSet(T element1, T element2, T element3, T element4, T element5, T element6, T element7, T element8, T element9)
	{
		m_pHead = new CSetNode(0);
		m_Size = 0;

		T elements[] = { element1, element2, element3, element4, element5, element6, element7, element8, element9 };
		BuildListFromVariableConstructor(9, elements);
	}

private:
	void BuildListFromVariableConstructor(size_t numOfElements, T values[])
	{
		for (int i = 0; i < numOfElements; i++)
		{
			PushBack(values[i]);
		}
	}

public:

	T& operator[](int index)
	{
		CSetNode<T>* currentNode = m_pHead;
		for (int i = 0; i < index; i++)
			currentNode = currentNode->m_pNext;

		return currentNode->m_pData;
	}

	T Front()
	{
		if (m_pHead)
			return m_pHead->m_pData;

		LOG_ERROR("CSet->Front() - Attempting to access data that does not exist, returning -1");
		return -1;
	}

	T Back()
	{
		CSetNode<T>* currentNode = m_pHead;
		while (currentNode->m_pNext != nullptr)
			currentNode = currentNode->m_pNext;

		return currentNode->m_pData;
	}

	void PushFront(T val)
	{
		CSetNode<T>* newValue = new CSetNode(val);
		newValue->m_pNext = m_pHead;
		m_pHead = newValue;
		m_Size++;
	}

	void PushBack(T val)
	{
		CSetNode<T>* currentPtr = m_pHead;
		bool duplicateFound = false;

		if (m_Size == 0)
		{
			m_pHead = new CSetNode(val);
			m_Size++;
			return;
		}

		while (currentPtr->m_pNext != nullptr)
		{
			if (currentPtr->m_pData == val)
			{
				duplicateFound = true;
				return;
			}
			currentPtr = currentPtr->m_pNext;
		}

		currentPtr->m_pNext = new CSetNode(val);
		m_Size++;
	}

	T PopFront()
	{
		CSetNode<T>* poppedNode = m_pHead;

		m_pHead = poppedNode->m_pNext;
		T poppedValue = poppedNode->m_pData;
		delete(poppedNode);

		m_Size--;
		return poppedValue;
	}

	T PopBack()
	{
		CSetNode<T>* newLastNode = m_pHead;

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
		CSetNode<T>* newPosIndexSub1 = m_pHead;

		// Lists are one-indexed it appears
		size_t index = 1;
		while (index != (--newPos))
			newPosIndexSub1 = newPosIndexSub1->m_pNext;

		// Save off a copy to the next element for later
		CSetNode<T>* linkToBeAdjusted = new CSetNode<T>(*newPosIndexSub1->m_pNext);

		// Insert a new node at the element's position
		newPosIndexSub1->m_pNext = new CSetNode<T>(element);

		// Insert more elements if provided
		size_t elementsAdded = 1;
		CSetNode<T>* newElement = newPosIndexSub1->m_pNext;
		while (elementsAdded != numElements)
		{
			CSetNode<T>* newNode = new CSetNode<T>;
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
	/* The current number of elements in the set (not necessarily the capacity) */
	size_t m_Size;

	/* The head node of the set. */
	CSetNode<T>* m_pHead;
};