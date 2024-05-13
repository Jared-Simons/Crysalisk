//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Systems/IEngine.h>
#include <Core/Logging.h>
#include <Clock.h>

#include "Test.h"

#include <CList.h>

bool PerformUnitTests()
{
	MyTests->PerformTests();
	
	return true;
}

int main()
{
	// Running unit tests in headless mode as we do not require rendering or input.
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = true;

	if (!Engine->Engine_Initialize(engineConfig))
		return -1;

	PerformUnitTests();

	CList<int> myIntList = { 12, 45, 8, 6 };
	for (int i = 0; i < myIntList.GetSize(); i++)
		LOG_WARN("%d", myIntList[i]);

	Engine->Engine_Shutdown();

	return 0;
}