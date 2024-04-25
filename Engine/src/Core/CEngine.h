//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================
#pragma once

#include <stdio.h>
#include "Systems/IEngine.h"

class CEngine : public IEngine
{
public:
	bool	EngineInitialize() override
	{
		printf("EngineInitialize() called!\n");
		return true;
	}

	bool	EngineRun() override
	{
		//printf("EngineRun() called!\n");
		return true;
	}

	void	EngineShutdown() override
	{
		printf("EngineShutdown() called\n");
	}
};