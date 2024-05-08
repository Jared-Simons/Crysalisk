//=============================================================================
// Author: Jared Simons
// Date Created: 5/8/2024
// 
// Purpose: A Custom Linked-List.
//=============================================================================
#pragma once

template<typename T>
class CListNode
{
public:
	CList* m_pNext;
	T m_Data;
};

template<typename T>
class CList
{
public:
	CList() {}

private:
	CListNode* m_pHead;
};

