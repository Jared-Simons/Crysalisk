//=============================================================================
// Author: Jared Simons
// Date Created: 7/3/2026
// 
// Purpose: The entrypoint for the engine and applications.
//=============================================================================
#pragma once

#include <Defines.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>

extern "C" __declspec(dllimport) i32 Crysalisk_Main(ApplicationConfig_t& pAppConfig);
extern bool CreateApplication(ApplicationConfig_t* pAppConfig);

i32 main(i32 argc, char** argv)
{
	ApplicationConfig_t appConfig{};
	CreateEngineInstance(&appConfig);

	// Create application instance.
	CreateApplication(&appConfig);

	return Crysalisk_Main(appConfig);
}
