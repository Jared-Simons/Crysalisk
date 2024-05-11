//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Engine Interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

struct EngineConfig_t
{
	// Set this to true to run the engine without rendering or platform faculties.
	// This is useful for unit tests and dedicated servers that do not require input.
	bool isHeadless = false;
};

class ENGINE_API IEngine
{
public:
	virtual bool	Engine_Initialize(EngineConfig_t engineConfig) = 0;
	virtual bool	Engine_Run() = 0;
	virtual void	Engine_Shutdown() = 0;
};

// Global Engine Layer Interface
ENGINE_API extern IEngine* Engine;

