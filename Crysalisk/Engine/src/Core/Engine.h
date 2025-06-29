//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Engine interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "Defines.h"

//-------------------------------------------------------------------------
// Engine configuration, to be passed in by the calling application.
//-------------------------------------------------------------------------
struct EngineConfig_t
{
	// Set this to true to run the engine without rendering or platform faculties.
	// This is useful for unit tests and dedicated servers that do not require input.
	bool isHeadless = false;
};

//-------------------------------------------------------------------------
// Interface for the core engine.
//-------------------------------------------------------------------------
class IEngine
{
public:
	// These get called by the the application using the engine.

	// Initialize the engine and it's required subsystems.
	virtual bool	Initialize(EngineConfig_t engineConfig) = 0;

	// Runs an engine frame.
	virtual bool	Run() = 0;

	// Shuts down the engine.
	virtual void	Shutdown() = 0;
};

// Global interface to the engine context.
extern IEngine* g_pEngine;

ENGINE_API bool WindowResizeHandler(unsigned int width, unsigned int height);