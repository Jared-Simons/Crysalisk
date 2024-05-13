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
		RegisterTestFunc(CreateAndDestroyLinkedList);
		RegisterTestFunc(InitializeLinkedList);
	}

private:
};

extern UnitTestGroup* g_LinkedListTests;
