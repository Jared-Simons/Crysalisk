//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Defines.h>

#include <Containers/CFreeList.h> // TODO: REMOVE THESE!
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

	Utl::CFreeList freelist(200);
	size_t freelistRequirement = freelist.GetInternalMemoryRequirement();
	void* freelistBlock = malloc(freelistRequirement);
	freelist.Init(freelistBlock);

	size_t offset1 = freelist.Allocate(10);
	size_t offset2 = freelist.Allocate(50);
	size_t offset3 = freelist.Allocate(20);

	freelist.Free(10, offset1);
	freelist.Free(20, offset3);
	size_t offset4 = freelist.Allocate(5);

	free(freelistBlock);

	PerformUnitTests();

	return 0;
}