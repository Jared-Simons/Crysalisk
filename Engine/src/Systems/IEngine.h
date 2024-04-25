//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Engine Interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

class ENGINE_API IEngine
{
public:
	virtual bool	EngineInitialize() = 0;
	virtual bool	EngineRun() = 0;
	virtual void	EngineShutdown() = 0;
};

ENGINE_API extern IEngine* Engine;