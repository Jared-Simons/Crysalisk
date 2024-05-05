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
	virtual bool	Engine_Initialize() = 0;
	virtual bool	Engine_Run() = 0;
	virtual void	Engine_Shutdown() = 0;
};

// Global Engine Layer Interface
ENGINE_API extern IEngine* Engine;

