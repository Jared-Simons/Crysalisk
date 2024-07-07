#include "DynamicAllocatorTests.h"

#include <Memory/DynamicAllocator.h>


int DynamicAllocator_SingleAllocationAllSpace()
{
	const size_t totalAllocatorSize = 1024 + (ALLOC_BLOCK_HEADER_SIZE);

	DynamicAllocator allocator(totalAllocatorSize);
	if (allocator.GetFreeSpace() != totalAllocatorSize)
		return FAIL;

	// Allocate the whole block
	void* block = allocator.Allocate(1024);

	if (allocator.GetFreeSpace() != 0 || !block)
		return FAIL;

	// Free the allocation
	allocator.Free(block);

	if (allocator.GetFreeSpace() != totalAllocatorSize)
		return FAIL;

	return PASS;
}

int DynamicAllocator_MultiAllocationAllSpace()
{
	const size_t totalAllocatorSize = 1024 + (ALLOC_BLOCK_HEADER_SIZE * 3);

	DynamicAllocator allocator(totalAllocatorSize);
	if (allocator.GetFreeSpace() != totalAllocatorSize)
		return FAIL;

	// Allocate part of the block
	void* block = allocator.Allocate(256);
	if (allocator.GetFreeSpace() != (768 + (ALLOC_BLOCK_HEADER_SIZE * 2)) || !block)
		return FAIL;

	// Allocate another part of the block
	void* block2 = allocator.Allocate(512);
	if (allocator.GetFreeSpace() != (256 + (ALLOC_BLOCK_HEADER_SIZE * 1)) || !block2)
		return FAIL;

	// Allocate the last part of the block
	void* block3 = allocator.Allocate(256);
	if (allocator.GetFreeSpace() != 0 || !block3)
		return FAIL;

	// Free all allocations, out of order
	allocator.Free(block3);
	if (allocator.GetFreeSpace() != (256 + (ALLOC_BLOCK_HEADER_SIZE * 1)))
		return FAIL;

	allocator.Free(block);
	if (allocator.GetFreeSpace() != (512 + (ALLOC_BLOCK_HEADER_SIZE * 2)))
		return FAIL;

	allocator.Free(block2);
	if (allocator.GetFreeSpace() != totalAllocatorSize)
		return FAIL;

	return PASS;
}

int DynamicAllocator_MultiAllocationOverAllocate()
{
	const size_t totalAllocatorSize = 1024 + (ALLOC_BLOCK_HEADER_SIZE * 3);

	DynamicAllocator allocator(totalAllocatorSize);
	if (allocator.GetFreeSpace() != totalAllocatorSize)
		return FAIL;

	// Allocate part of the block
	void* block = allocator.Allocate(256);
	if (allocator.GetFreeSpace() != (768 + (ALLOC_BLOCK_HEADER_SIZE * 2)) || !block)
		return FAIL;

	// Allocate another part of the block
	void* block2 = allocator.Allocate(512);
	if (allocator.GetFreeSpace() != (256 + (ALLOC_BLOCK_HEADER_SIZE * 1)) || !block2)
		return FAIL;

	// Allocate the last part of the block
	void* block3 = allocator.Allocate(256);
	if (allocator.GetFreeSpace() != 0 || !block3)
		return FAIL;

	// Attempt one more allocation, deliberately trying to overflow
	LOG_WARN("NOTE: The following warning and errors are intentionally caused by this test.");
	void* failBlock = allocator.Allocate(256);

	if (failBlock != nullptr)
		return FAIL;

	// Verify free space
	if (allocator.GetFreeSpace() != 0)
		return FAIL;

	return PASS;
}

static CDynamicAllocatorTests s_DynamicAllocatorUnitTests("Dynamic Allocator Tests");
UnitTestGroup* g_DynamicAllocatorTests = &s_DynamicAllocatorUnitTests;
