//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Systems/IEngine.h>
#include <Systems/IMemory.h>
#include <Core/Logging.h>
#include <Clock.h>

#include <Containers/LinkedListTests.h>
#include <Memory/LinearAllocatorTests.h>
#include <Memory/DynamicAllocatorTests.h>

bool PerformUnitTests()
{
	//g_LinearAllocatorTests->PerformTests();
	//g_LinkedListTests->PerformTests();
	g_DynamicAllocatorTests->PerformTests();
	
	return true;
}

int main()
{
	// Running unit tests in headless mode as we do not require rendering or input.
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = true;

	if (!Engine->Engine_Initialize(engineConfig))
		return -1;

	LOG_VERBOSE("\n\n\n\n\n");

	PerformUnitTests();

	//gpMemorySystem->PrintMemoryStats();
	LOG_VERBOSE("\n\n\n\n\n");

	Engine->Engine_Shutdown();

	return 0;
}