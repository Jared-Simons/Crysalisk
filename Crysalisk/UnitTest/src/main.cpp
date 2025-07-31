//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Defines.h>

//#include <Containers/CFreeList.h> // TODO: REMOVE THESE!
#include <Memory/DynamicAllocator.h>
#include <malloc.h>

#include <Core/Clock.h>
#include "Containers/CVectorTests.h"

bool PerformUnitTests()
{	
	g_pCVectorUnitTests->PerformTests();

	return true;
}

int main()
{
	// Running unit tests in headless mode as we do not require rendering or input.
	/*EngineConfig_t engineConfig{};
	engineConfig.isHeadless = true;
	Engine pEngineState;

	if (!Engine::Engine_Initialize(engineConfig, &pEngineState))
		return -1;

	LOG_VERBOSE("\n\n\n\n\n");

	PerformUnitTests();

	//gpMemorySystem->PrintMemoryStats();
	LOG_VERBOSE("\n\n\n\n\n");*/

	/*void* pMemoryBlock = malloc(sizeof(int) * 20);
	Utl::DynamicAllocator dynamicAllocator{};
	dynamicAllocator.InitAllocatorInternals(sizeof(int) * 20, pMemoryBlock);
	size_t freelistMemoryRequirement = dynamicAllocator.GetInternalFreelistMemoryRequirement();
	void* pFreelistBlock = malloc(freelistMemoryRequirement);

	dynamicAllocator.InitFreelist(pFreelistBlock);

	void* pBlock1 = dynamicAllocator.Allocate(sizeof(int));
	int* pInt = (int*)pBlock1;
	*pInt = 100;
	LOG_INFO("%d", *pInt);

	dynamicAllocator.Free(pBlock1);

	free(pFreelistBlock);
	free(pMemoryBlock);*/

	PerformUnitTests();

	return 0;
}