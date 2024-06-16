// CREDIT - These tests borrowed from Kohi unit tests.
// (https://github.com/travisvroman/kohi/blob/main/tests/src/containers/freelist_tests.c)

#include "FreeListTests.h"

#include <Containers/CFreeList.h>


int CreateAndDestroyFreelist()
{
	LOG_DEBUG("The following warning message is intentional.");

	CFreeList* FreeList = new CFreeList(40);

	if (FreeList == nullptr)
		return FAIL;

	if (FreeList->GetTotalSize() != FreeList->GetFreeSpace())
		return FAIL;

	delete FreeList;
	return PASS;
}


int FreeListAllocateOneAndFreeOne()
{
	CFreeList* FreeList = new CFreeList(512);

	if (FreeList == nullptr)
		return FAIL;

	if (FreeList->GetTotalSize() != FreeList->GetFreeSpace())
		return FAIL;

	size_t offset = UINT_MAX; // Placeholder until we implement an invalid value
	bool result = FreeList->Allocate(64, &offset);

	// Verify that the allocation was successful
	if (!result || offset != 0)
		return FAIL;

	// Verify that the correct amount of space is free.
	size_t freeSpace = FreeList->GetFreeSpace();

	if (freeSpace != FreeList->GetTotalSize() - 64)
		return FAIL;

	// Now free the allocation.
	result = FreeList->Free(64, offset);

	if (!result)
		return FAIL;

	delete FreeList;
	return PASS;
}


int FreeListAllocateMultipleAndFreeMultiple()
{
	CFreeList* FreeList = new CFreeList(512);

	if (FreeList == nullptr)
		return FAIL;

	if (FreeList->GetTotalSize() != FreeList->GetFreeSpace())
		return FAIL;

	size_t offset = UINT_MAX; // Placeholder until we implement an invalid value
	bool result = FreeList->Allocate(64, &offset);

	if (!result || offset != 0)
		return FAIL;

	// Allocate some more space.
	size_t offset2 = UINT_MAX;
	result = FreeList->Allocate(64, &offset2);

	if (!result || offset2 != 64)
		return FAIL;

	// Allocate one more space.
	size_t offset3 = UINT_MAX;
	result = FreeList->Allocate(64, &offset3);

	if (!result || offset3 != 128)
		return FAIL;

	// Verify that the correct amount of space is free.
	if (FreeList->GetFreeSpace() != FreeList->GetTotalSize() - 192)
		return FAIL;

	// Now free the first block and verify space.
	result = FreeList->Free(64, offset);
	if (!result || (FreeList->GetFreeSpace() != FreeList->GetTotalSize() - 128))
		return FAIL;

	// Free the last block and verify space.
	result = FreeList->Free(64, offset3);
	if (!result || (FreeList->GetFreeSpace() != FreeList->GetTotalSize() - 64))
		return FAIL;

	// Free the middle block and verify space.
	result = FreeList->Free(64, offset2);
	if (!result || (FreeList->GetFreeSpace() != FreeList->GetTotalSize()))
		return FAIL;

	delete FreeList;
	return PASS;
}


int FreeListAllocateMultipleAndFreeMultipleVaryingSizes()
{
	CFreeList* FreeList = new CFreeList(512);

	if (FreeList == nullptr)
		return FAIL;

	if (FreeList->GetTotalSize() != FreeList->GetFreeSpace())
		return FAIL;

	// Allocate some space.
	size_t offset = UINT_MAX; // Placeholder until we implement an invalid value
	bool result = FreeList->Allocate(64, &offset);
	// Verify the allocation.
	if (!result || offset != 0)
		return FAIL;

	// Allocate some more space.
	size_t offset2 = UINT_MAX; // Placeholder until we implement an invalid value
	result = FreeList->Allocate(32, &offset2);
	// Verify the allocation.
	if (!result || offset2 != 64)
		return FAIL;

	// Allocate one more space.
	size_t offset3 = UINT_MAX; // Placeholder until we implement an invalid value
	result = FreeList->Allocate(64, &offset3);
	// Verify the allocation.
	if (!result || offset3 != 96)
		return FAIL;

	// Verify the correct amount of space is free.
	if (FreeList->GetTotalSize() - 160 != FreeList->GetFreeSpace())
		return FAIL;

	// Free the middle block.
	result = FreeList->Free(32, offset2);
	if (!result || (FreeList->GetTotalSize() - 128 != FreeList->GetFreeSpace()))
		return FAIL;

	// Allocate some more space, this time larger than the old middle block. This should have a new offset
	// at the end of the list.
	size_t offset4 = UINT_MAX;
	result = FreeList->Allocate(64, &offset4);
	if (!result || offset4 != 160) // Offset should be the end since it occupies more space than the old middle block.
		return FAIL;

	if (FreeList->GetFreeSpace() != FreeList->GetTotalSize() - 192)
		return FAIL;

	// Free the first block and verify space.
	result = FreeList->Free(64, offset);
	if (!result || (FreeList->GetTotalSize() - 128 != FreeList->GetFreeSpace()))
		return FAIL;

	// Free the last block and verify space.
	result = FreeList->Free(64, offset3);
	if (!result || (FreeList->GetTotalSize() - 64 != FreeList->GetFreeSpace()))
		return FAIL;

	// Free the middle (now end) block and verify space.
	result = FreeList->Free(64, offset4);
	if (!result || (FreeList->GetTotalSize() != FreeList->GetFreeSpace()))
		return FAIL;

	delete FreeList;
	return PASS;
}


int FreeListAllocateToFullAndFailToAllocateMore()
{
	CFreeList* FreeList = new CFreeList(512);

	if (FreeList == nullptr)
		return FAIL;

	if (FreeList->GetTotalSize() != FreeList->GetFreeSpace())
		return FAIL;

	// Allocate all space.
	size_t offset = UINT_MAX;
	bool result = FreeList->Allocate(512, &offset);
	if (!result || offset != 0)
		return FAIL;

	// Verify that the correct amount of space is free.
	if (FreeList->GetFreeSpace() != 0)
		return FAIL;

	// Now try allocating some more
	size_t offset2 = UINT_MAX;
	LOG_DEBUG("The following warning message is intentional.");
	result = FreeList->Allocate(64, &offset2);
	if (result)
		return FAIL;

	// Verify that the correct amount of space is free.
	if (FreeList->GetFreeSpace() != 0)
		return FAIL;

	delete FreeList;
	return PASS;
}


struct AllocData_t
{
	size_t size;
	size_t offset;
};


int FreeListUtilAllocate(CFreeList* list, AllocData_t* data, size_t* currentlyAllocated, size_t totalListSize)
{
	// Allocate some space
	data->offset = UINT_MAX;
	bool result = list->Allocate(data->size, &data->offset);
	// Verify that the allocation was successful
	if (!result || (data->offset == UINT_MAX))
		return FAIL;

	// Track it.
	*currentlyAllocated += data->size;

	// Verify free space.
	if (list->GetFreeSpace() != list->GetTotalSize() - *currentlyAllocated)
		return FAIL;

	return PASS;
}


int FreeListUtilFree(CFreeList* list, AllocData_t* data, size_t* currentlyAllocated, size_t totalListSize)
{
	// Free the block and verify space.
	bool result = list->Free(data->size, data->offset);
	if (!result)
		return FAIL;

	// Track it.
	*currentlyAllocated -= data->size;

	// Verify free space.
	if (list->GetFreeSpace() != list->GetTotalSize() - *currentlyAllocated)
		return FAIL;

	// Reset the data.
	data->offset = UINT_MAX;

	return PASS;
}


static CFreeListUnitTests s_FreeListUnitTests("Free List Tests");
UnitTestGroup* g_FreeListTests = &s_FreeListUnitTests;

