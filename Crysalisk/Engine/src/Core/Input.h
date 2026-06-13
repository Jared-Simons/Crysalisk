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
	int m_XPos;
	int m_YPos;
	u32 m_bitsMouseButtons;
};

//-------------------------------------------------------------------------
// The interface to the input system.
//-------------------------------------------------------------------------
class IInputSystem
{
public:
	static void		AllocateInputSystemState(IInputSystem** ppSystemState);

	virtual bool	Initialize(Platform::IPlatformWindow& pEngineWindow, class IEventSystem* pEngineEventHandler) = 0;
	virtual void	Shutdown() = 0;
	virtual void	Update() = 0;

	virtual void	ProcessKeyInput(const int processedKeyInput, bool pressed) = 0;
	virtual void	ProcessMouseButtonInput(byte mouseButtonFlags, bool pressed) = 0;
	virtual void	ProcessMousePositionInput(const int mouseXPos, const int mouseYPos) = 0;

	virtual bool	IsKeyPressed(const int key) const = 0;
	virtual void	GetCurrentMousePos(int& iX, int& iY) = 0;
};

//-----------------------------------------------------------------------------
// API for external libraries to access our callbacks.
//-----------------------------------------------------------------------------
void API_ProcessKeyInput(int processedKeyInput, bool pressed);
void API_ProcessMouseButtonInput(u32 mouseButtonFlags, bool bPressed);
void API_ProcessMouseMoveInput(const int mouseXPos, const int mouseYPos);