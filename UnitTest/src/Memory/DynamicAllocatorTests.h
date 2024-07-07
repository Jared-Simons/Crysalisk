// CREDIT: These tests borrowed from the Kohi Game Engine :)
// (https://github.com/travisvroman/kohi/blob/main/tests/src/memory/dynamic_allocator_tests.c)

#pragma once

#include "UnitTestType.h"

// Forward Declarations
class UnitTestGroup;

// Test Functions
int DynamicAllocator_SingleAllocationAllSpace();
int DynamicAllocator_MultiAllocationAllSpace();
int DynamicAllocator_MultiAllocationOverAllocate();

class CDynamicAllocatorTests : public UnitTestGroup
{
public:
	CDynamicAllocatorTests(const char* debugTestName)
		:UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(DynamicAllocator_SingleAllocationAllSpace, "Dynamic Allocator - Single Allocation All Space");
		RegisterTestFunc(DynamicAllocator_MultiAllocationAllSpace, "Dynamic Allocator - Multi Allocation All Space");
		RegisterTestFunc(DynamicAllocator_MultiAllocationOverAllocate, "Dynamic Allocator - Multi Allocation Over Allocate");
	}
};

extern UnitTestGroup* g_DynamicAllocatorTests;

