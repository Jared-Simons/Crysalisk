//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: The interface for an application that uses the Crysalisk Engine.
//=============================================================================
#pragma once

struct ApplicationConfig_t
{
	class ICrysApplication* pApplication{ nullptr };
};

class ICrysApplication
{
public:
	virtual bool Application_Init() = 0;
	virtual bool Application_Run() = 0;
	virtual void Application_Shutdown() = 0;
};
