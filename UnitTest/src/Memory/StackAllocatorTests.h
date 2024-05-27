// CREDIT: These tests borrowed from the Kohi Game Engine :)
// (https://github.com/travisvroman/kohi/blob/main/tests/src/memory/linear_allocator_tests.c)

#pragma once

#include "UnitTestType.h"

// Forward Declarations
class UnitTestGroup;

// Test Functions
int CreateAndDestroyStackAllocator();
int StackAllocatorSingleAllocationAllSpace();
int StackAllocatorMultiAllocationAllSpace();
int StackAllocatorMultiAllocationOverAllocate();
int StackAllocatorMultiAllocationAllSpaceThenFree();

class CStackAllocatorTests : public UnitTestGroup
{
public:
	CStackAllocatorTests(const char* debugTestName)
		: UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(CreateAndDestroyStackAllocator, "Create and Destroy Stack Allocator");
		RegisterTestFunc(StackAllocatorSingleAllocationAllSpace, "Stack Allocator - Single Allocation All Space");
		RegisterTestFunc(StackAllocatorMultiAllocationAllSpace, "Stack Allocator - Multi-Allocation All Space");
		RegisterTestFunc(StackAllocatorMultiAllocationOverAllocate, "Stack Allocator - Multi-Allocation Over Allocate");
		RegisterTestFunc(StackAllocatorMultiAllocationAllSpaceThenFree, "Stack Allocator - Multi-Allocation All Space Then Free");
	}
};

extern UnitTestGroup* g_StackAllocatorTests;

