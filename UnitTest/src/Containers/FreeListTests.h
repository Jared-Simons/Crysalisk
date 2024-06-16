#pragma once

#include "UnitTestType.h"

// Forward Declarations
class UnitTestGroup;

// Test Functions
int CreateAndDestroyFreelist();
int FreeListAllocateOneAndFreeOne();
int FreeListAllocateMultipleAndFreeMultiple();
int FreeListAllocateMultipleAndFreeMultipleVaryingSizes();
int FreeListAllocateToFullAndFailToAllocateMore();


class CFreeListUnitTests : public UnitTestGroup
{
public:
	CFreeListUnitTests(const char* debugTestName)
		: UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(CreateAndDestroyFreelist, "Create And Destroy Free list");
		RegisterTestFunc(FreeListAllocateOneAndFreeOne, "Free list - Allocate one and Free one");
		RegisterTestFunc(FreeListAllocateMultipleAndFreeMultiple, "Free list - Allocate multiple and Free multiple");
		RegisterTestFunc(FreeListAllocateMultipleAndFreeMultipleVaryingSizes, "Free list - Allocate multiple and free multiple - Varying Sizes.");
		RegisterTestFunc(FreeListAllocateToFullAndFailToAllocateMore, "Free list - Allocate to full and fail to allocate more");
	}
};

extern UnitTestGroup* g_FreeListTests;

