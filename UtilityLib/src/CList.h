//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: A Custom Linked-List.
//=============================================================================
#pragma once

#include <stdarg.h>

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
		, m_Data{} {}

	CListNode(T val)
		: m_pNext{ nullptr }
		, m_Data{ val } {}

	CListNode* m_pNext;
	T m_Data;
};

template<typename T>
class CList
{
public:

	//==============================================
	// BIG WALL OF CONSTRUCTORS
	//==============================================

	CList()
		: m_Size{ 0 }
		, m_pHead{ new CListNode<T>() } {}

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
		for (int i = 0; i < numOfElements; i++)
		{
			PushBack(values[i]);
		}
	}

public:

	void PushBack(T val)
	{
		CListNode<T>* currentPtr = m_pHead;
		if (m_Size == 0)
		{
			m_pHead->m_Data = val;
			m_Size++;
			return;
		}

		while (currentPtr->m_pNext != nullptr)
			currentPtr = currentPtr->m_pNext;

		currentPtr->m_pNext = new CListNode(val);
		m_Size++;
	}

	T& operator[](int index)
	{
		CListNode<T>* currentNode = m_pHead;
		for (int i = 0; i < index; i++)
			currentNode = currentNode->m_pNext;

		return currentNode->m_Data;
	}

	size_t GetSize() const { return m_Size; }

private:
	/* The current number of elements in the list (not necessarily the capacity) */
	size_t m_Size;

	/* The head node of the linked list. */
	CListNode<T>* m_pHead;
};

