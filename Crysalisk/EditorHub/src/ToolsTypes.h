//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: Tool type declarations.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Platform/Platform.h>

#if defined EDITOR_HUB_DLL
	#define EDITOR_HUB_API __declspec(dllexport)
#else
	#define EDITOR_HUB_API __declspec(dllimport)
#endif

class EDITOR_HUB_API IToolApp
{
public:
	virtual bool	Tool_Init() = 0;
	virtual bool	Tool_Run() = 0;
	virtual void	Tool_Shutdown() = 0;

	Platform::ISysModule* pToolModule{ nullptr };
};

DLL_EXPORT class ICrysApplication* GetApplicationState();
