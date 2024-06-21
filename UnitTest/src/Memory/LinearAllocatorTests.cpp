#include "LinearAllocatorTests.h"

#include <Memory/LinearAllocator.h>


int CreateAndDestroyLinearAllocator()
{
	LinearAllocator linearAlloc(1 * sizeof(unsigned long long));

	if (linearAlloc.GetMemory() == nullptr)
		return FAIL;

	if (linearAlloc.GetAllocatorSize() != sizeof(unsigned long long))
		return FAIL;

	if (linearAlloc.GetBytesAllocated() != 0)
		return FAIL;

	return PASS;
}


int LinearAllocatorSingleAllocationAllSpace()
{
	LinearAllocator linearAlloc(1 * sizeof(unsigned long long));

	// Single allocation.
	void* block = linearAlloc.allocate(sizeof(unsigned long long));

	// Validate it
	if (linearAlloc.GetMemory() == nullptr)
		return FAIL;

	if (linearAlloc.GetBytesAllocated() != sizeof(unsigned long long))
		return FAIL;

	return PASS;
}


int LinearAllocatorMultiAllocationAllSpace()
{
	unsigned long long maxAllocs = 1024;
	LinearAllocator LinearAlloc(maxAllocs * sizeof(unsigned long long));

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = LinearAlloc.allocate(sizeof(unsigned long long));

		// Validate it
		if (block == nullptr)
			return FAIL;

		// This extra type casting is being done purely to avoid compiler warnings/messages.
		size_t numBytesAllocated = LinearAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	return PASS;
}


int LinearAllocatorMultiAllocationOverAllocate()
{
	unsigned long long maxAllocs = 1024;
	LinearAllocator LinearAlloc(maxAllocs * sizeof(unsigned long long));

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = LinearAlloc.allocate(sizeof(unsigned long long));

		// Validate it
		if (block == nullptr)
			return FAIL;

		size_t numBytesAllocated = LinearAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	LOG_DEBUG("Note: The following error is intentionally caused by this test.");

	// Ask for one more allocation. Should error and return 0.
	block = LinearAlloc.allocate(sizeof(char));

	// Validate the nullptr.
	if (block != nullptr)
		return FAIL;

	size_t numBytesAllocated = LinearAlloc.GetBytesAllocated();
	if (numBytesAllocated != sizeof(unsigned long long) * maxAllocs)
		return FAIL;

	return PASS;
}


int LinearAllocatorMultiAllocationAllSpaceThenFree()
{
	unsigned long long maxAllocs = 1024;
	LinearAllocator LinearAlloc(maxAllocs * sizeof(unsigned long long));

	// Multiple allocations - full.
	void* block;
	for (int i = 0; i < maxAllocs; i++)
	{
		block = LinearAlloc.allocate(sizeof(unsigned long long));

		// Validate it
		if (block == nullptr)
			return FAIL;

		size_t numBytesAllocated = LinearAlloc.GetBytesAllocated();
		size_t currentBytesExpected = static_cast<size_t>((i + 1)) * sizeof(unsigned long long);
		if (numBytesAllocated != currentBytesExpected)
			return FAIL;
	}

	// Validate that the pointer is reset.
	LinearAlloc.free();

	if (LinearAlloc.GetBytesAllocated() != 0)
		return FAIL;

	return PASS;
}


static CLinearAllocatorTests s_LinearedAllocatorUnitTests("Linear Allocator Tests");
UnitTestGroup* g_LinearAllocatorTests = &s_LinearedAllocatorUnitTests;

