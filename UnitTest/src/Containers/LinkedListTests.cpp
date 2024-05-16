#include "LinkedListTests.h"

#include <Containers/CList.h>

int CreateAndDestroyLinkedList()
{
	CList<int>* myList = new CList<int>;

	if (myList == nullptr)
		return FAIL;

	delete myList;

	return PASS;
}

int InitializeLinkedList()
{
	CList<int> myList{ 0, 2, 4, 6, 8, 10, 12, 14, 16 };

	if (myList.GetSize() == 9 && myList[4] == 8)
		return PASS;

	return FAIL;
}

static CListUnitTests s_LinkedListUnitTests("Linked List Tests");
UnitTestGroup* g_LinkedListTests = &s_LinkedListUnitTests;

