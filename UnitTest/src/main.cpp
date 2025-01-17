//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Defines.h>

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
	PerformUnitTests();

	return 0;
}