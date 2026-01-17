//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Engine interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "Defines.h"
#include <Platform/Platform.h>

//-------------------------------------------------------------------------
// Engine configuration, to be passed in by the calling application.
//-------------------------------------------------------------------------
struct EngineConfig_t
{
	// Set this to true to run the engine without rendering or platform faculties.
	// This is useful for unit tests and dedicated servers that do not require input.
	bool isHeadless = false;

	// The application configuration for the app using the engine.
	struct ApplicationConfig_t* pAppConfig{};
};

//-------------------------------------------------------------------------
// Interfaces of engine systems.
//-------------------------------------------------------------------------
struct EngineInterfaces_t
{
	class LogSystem*		pLogger{ nullptr };
	class IEventSystem*		pEngineEventHandler{ nullptr };
	class IInputSystem*		pInputSystem{ nullptr };
	class IRenderer*		pRenderer{ nullptr };
	class IDebugRenderer*	pDebugRenderer{ nullptr };
	class IJobSystem*		pJobSystem{ nullptr };
	class ITextureSystem*	pTextureSystem{ nullptr };
	class IShaderSystem*	pShaderSystem{ nullptr };
	class IMaterialSystem*	pMaterialSystem{ nullptr };
	class IMeshSystem*		pMeshSystem{ nullptr };
	class IFontSystem*		pFontSystem{ nullptr };

	class CMap*				pCurrentScene{ nullptr };
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

	// Gets a pointer to the engine's main window.
	virtual Platform::IPlatformWindow*	GetEngineWindow() const = 0;

	// Gets a pointer to engine subsystems.
	virtual inline const EngineInterfaces_t*		GetEngineInterfaces() const = 0;
};

// Global interface to the engine context.
extern IEngine* g_pEngine;

ENGINE_API bool WindowResizeHandler(unsigned int width, unsigned int height);
void WindowClosedHandler();