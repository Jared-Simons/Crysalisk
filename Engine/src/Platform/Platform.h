//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

bool	Platform_Initialize();
bool	Platform_Update();
void	Platform_Shutdown();

bool	Platform_ProcessMessages();