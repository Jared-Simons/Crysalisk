#pragma once

#include "UnitTestType.h"

// Forward Declarations
class UnitTestGroup;

// Test Functions
int CreateAndDestroyLinkedList();
int InitializeLinkedList();


class CListUnitTests : public UnitTestGroup
{
public:
	CListUnitTests(const char* debugTestName)
		: UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(CreateAndDestroyLinkedList, "Create and Destroy A Linked List");
		RegisterTestFunc(InitializeLinkedList, "Initialize Linked List");
	}

private:
};

extern UnitTestGroup* g_LinkedListTests;
