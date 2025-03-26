//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: Provides the Input System interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include <Core/InputTypes.h>
#include <Defines.h>
#include <Platform/Platform.h>

//-------------------------------------------------------------------------
// The state of a single key in the input manager.
//-------------------------------------------------------------------------
struct KeyState_t
{
	// Is the key currently being pressed? (This gets reset each update if no longer being pressed).
	bool m_bPressed;
	// What is the string representation of this key?
	char m_Char;
};

//-------------------------------------------------------------------------
// The state of the mouse in the input manager.
//-------------------------------------------------------------------------
struct MouseState_t
{
	// The position of the cursor on the screen.
	float m_XPos;
	float m_YPos;
	unsigned int m_MouseFlags;
};

//-------------------------------------------------------------------------
// The interface to the event system.
//-------------------------------------------------------------------------
class IInputSystem
{
public:
	virtual bool	Initialize(Platform::IPlatformWindow& pEngineWindow) = 0;
	virtual void	Shutdown() = 0;
	virtual void	Update() = 0;

	virtual void	ProcessKeyInput(const int processedKeyInput, bool pressed) = 0;
	virtual void	ProcessMouseButtonInput(byte mouseButtonFlags, bool pressed) = 0;
	virtual void	ProcessMousePositionInput(const float mouseXPos, const float mouseYPos) = 0;

	virtual bool	IsKeyPressed(const int key) const = 0;
};

// A note on interfaces:
// Any code that wants to access the input system from within the engine.dll can include this file and use g_pInputSystem_Internal.
// Any code in another library must link with Interfaces.lib to get access to the engine's shared interfaces. Then include both Input.h AND Interfaces.h,
// to get access to the g_pInputSystem interface.
ENGINE_API extern IInputSystem* g_pInputSystem_Internal;

//-----------------------------------------------------------------------------
// API for external libraries to access our callbacks.
//-----------------------------------------------------------------------------
void API_ProcessKeyInput(int processedKeyInput, bool pressed);
void API_ProcessMouseButtonInput(unsigned int mouseButtonFlags, bool bPressed);
void API_ProcessMouseMoveInput(const float mouseXPos, const float mouseYPos);