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

#include "Math/MathLib.h"
#include "Math/VectorTypes.h"
#include "Math/MatrixTypes.h"

bool PerformUnitTests()
{
	Vector2D vector{ 4.7, -6 };
	Vector2D vector2 = vector / 2;
	
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