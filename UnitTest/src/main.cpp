//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Defines.h>

#include <Core/Engine.h>
#include <Core/Logging.h>
#include <Core/Memory.h>
#include <Clock.h>

#include "Math/MathLib.h"
#include "Math/VectorTypes.h"
#include "Math/MatrixTypes.h"

bool PerformUnitTests()
{
	Vector2D vector{ 4.7f, -6.0f };
	Vector2D vector2 = vector / 2;
	
	return true;
}

i32 main()
{
	// Running unit tests in headless mode as we do not require rendering or input.
	EngineConfig_t engineConfig{};
	engineConfig.isHeadless = true;
	Engine pEngineState;

	if (!Engine::Engine_Initialize(engineConfig, &pEngineState))
		return -1;

	LOG_VERBOSE("\n\n\n\n\n");

	PerformUnitTests();

	//gpMemorySystem->PrintMemoryStats();
	LOG_VERBOSE("\n\n\n\n\n");

	return 0;
}