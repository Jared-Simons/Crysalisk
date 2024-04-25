//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================
#pragma once

#include "Systems/IEngine.h"

class CEngine : public IEngine
{
public:
	bool	Engine_Initialize() override;

	bool	Engine_Run() override;

	void	Engine_Shutdown() override;
};