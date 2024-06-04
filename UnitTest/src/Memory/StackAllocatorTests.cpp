#include "StackAllocatorTests.h"

#include <Memory/StackAllocator.h>


int CreateAndDestroyStackAllocator()
{
	StackAllocator<unsigned long long> stackAlloc(1);

	if (stackAlloc.GetMemory() == nullptr)
		return FAIL;

	if (stackAlloc.GetAllocatorSize() != sizeof(unsigned long long))
		return FAIL;

	if (stackAlloc.GetBytesAllocated() != 0)
		return FAIL;

	return PASS;
}


int StackAllocatorSingleAllocationAllSpace()
{
	StackAllocator<unsigned long long> stackAlloc(1);

	// Single allocation.
	void* block = stackAlloc.allocate();

	// Validate it
	if (stackAlloc.GetMemory() == nullptr)
		return FAIL;

	if (stackAlloc.GetBytesAllocated() != sizeof(unsigned long long))
		return FAIL;

	return PASS;
}


int StackAllocatorMultiAllocationAllSpace()
{
	unsigned long long maxAllocs = 1024;
	StackAllocator<unsigned long long> stackAlloc(maxAllocs);

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = stackAlloc.allocate();

		// Validate it
		if (block == nullptr)
			return FAIL;

		// This extra type casting is being done purely to avoid compiler warnings/messages.
		size_t numBytesAllocated = stackAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	return PASS;
}


int StackAllocatorMultiAllocationOverAllocate()
{
	unsigned long long maxAllocs = 1024;
	StackAllocator<unsigned long long> stackAlloc(maxAllocs);

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = stackAlloc.allocate();

		// Validate it
		if (block == nullptr)
			return FAIL;

		size_t numBytesAllocated = stackAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	LOG_DEBUG("Note: The following error is intentionally caused by this test.");

	// Ask for one more allocation. Should error and return 0.
	block = stackAlloc.allocate();

	// Validate the nullptr.
	if (block != nullptr)
		return FAIL;

	size_t numBytesAllocated = stackAlloc.GetBytesAllocated();
	if (numBytesAllocated != sizeof(unsigned long long) * maxAllocs)
		return FAIL;

	return PASS;
}


int StackAllocatorMultiAllocationAllSpaceThenFree()
{
	unsigned long long maxAllocs = 1024;
	StackAllocator<unsigned long long> stackAlloc(maxAllocs);

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = stackAlloc.allocate();

		// Validate it
		if (block == nullptr)
			return FAIL;

		size_t numBytesAllocated = stackAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	// Validate that the pointer is reset.
	stackAlloc.free();

	if (stackAlloc.GetBytesAllocated() != 0)
		return FAIL;

	return PASS;
}


static CStackAllocatorTests s_StackedAllocatorUnitTests("Stack Allocator Tests");
UnitTestGroup* g_StackAllocatorTests = &s_StackedAllocatorUnitTests;

