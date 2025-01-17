//=============================================================================
// Author: Jared Simons
// Date Created: 1/17/2025
// 
// Purpose: An interface for "Application Systems" that the application will track.
//=============================================================================
#pragma once

class IAppSystem
{
public:
	virtual bool	Initialize(void* pSysConfig) = 0;
};