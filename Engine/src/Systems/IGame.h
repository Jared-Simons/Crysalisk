//=============================================================================
// Author: Jared Simons
// Date Created: 5/4/2024
// 
// Purpose: Provides the Game interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "Defines.h"

class IGame
{
public:
	virtual bool	Game_Initialize() = 0;
	virtual bool	Game_Run() = 0;
	virtual void	Game_Shutdown() = 0;
};

