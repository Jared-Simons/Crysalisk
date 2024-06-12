// CREDIT: These tests borrowed from the Kohi Game Engine :)
// (https://github.com/travisvroman/kohi/blob/main/tests/src/memory/linear_allocator_tests.c)

#pragma once

#include "UnitTestType.h"

// Forward Declarations
class UnitTestGroup;

// Test Functions
int CreateAndDestroyLinearAllocator();
int LinearAllocatorSingleAllocationAllSpace();
int LinearAllocatorMultiAllocationAllSpace();
int LinearAllocatorMultiAllocationOverAllocate();
int LinearAllocatorMultiAllocationAllSpaceThenFree();

class CLinearAllocatorTests : public UnitTestGroup
{
public:
	CLinearAllocatorTests(const char* debugTestName)
		: UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(CreateAndDestroyLinearAllocator, "Create and Destroy Linear Allocator");
		RegisterTestFunc(LinearAllocatorSingleAllocationAllSpace, "Linear Allocator - Single Allocation All Space");
		RegisterTestFunc(LinearAllocatorMultiAllocationAllSpace, "Linear Allocator - Multi-Allocation All Space");
		RegisterTestFunc(LinearAllocatorMultiAllocationOverAllocate, "Linear Allocator - Multi-Allocation Over Allocate");
		RegisterTestFunc(LinearAllocatorMultiAllocationAllSpaceThenFree, "Linear Allocator - Multi-Allocation All Space Then Free");
	}
};

extern UnitTestGroup* g_LinearAllocatorTests;

