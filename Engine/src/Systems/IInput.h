//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: Provides the Input System Interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include <Memory/LinearAllocator.h>

// Forward Declarations
class LinearAllocator;

enum eKeyTable
{
	KEY_UNDEFINED = 0,

	KEY_CTRL = 17,

	KEY_SPACE = 32,

	KEY_LEFT = 37,
	KEY_UP,
	KEY_RIGHT,
	KEY_DOWN,

	KEY_A = 65,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	MAX_INPUT_KEYS
};

enum eMouseButtonTable
{
	MOUSE_BUTTON_UNDEFINED = -1,

	MOUSE_BUTTON_LEFTDOWN = 1,
	MOUSE_BUTTON_LEFTUP = 2,
	MOUSE_BUTTON_RIGHTDOWN = 3,
	MOUSE_BUTTON_RIGHTUP = 4,
	MOUSE_BUTTON_MIDDLEDOWN = 5,
	MOUSE_BUTTON_MIDDLEUP = 6,

	MAX_MOUSE_BUTTONS
};

// NOTE!!!: This string array MUST stay in parody with e_KeyTable!!
static const char* e_KeyTable_Strings[MAX_INPUT_KEYS] = {
	"UNDEFINED KEY!",

	"CTRL",

	"SPACE",

	"UP",
	"DOWN",

	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
};

static const char* e_MouseButtonTable_Strings[MAX_MOUSE_BUTTONS] = {
	"UNDEFINED",

	"LEFT",
	"LEFT",
	"RIGHT",
	"RIGHT",
	"MIDDLE",
	"MIDDLE",
};

struct KeyState_T
{
	bool m_Pressed;
	char m_Char;
};

struct MouseState_t
{
	float m_XPos;
	float m_YPos;
	bool m_MouseButtons[MAX_MOUSE_BUTTONS / 2];	// Divide by 2 to account for button up/down	TODO: Maybe pack these into an 8 bit integer? CURRENT IMPLEMENTATION WILL EVENTUALLY CAUSE AN ERROR!!!
};

class IInputSystem
{
public:
	static bool		InputSystem_Initialize(IInputSystem* statePtr, LinearAllocator* pAllocator);
	static void		InputSystem_Shutdown(IInputSystem* statePtr, bool bOwnsMemory = false);

	virtual bool	Initialize() = 0;
	virtual void	Shutdown() = 0;
	virtual void	Update() = 0;

	virtual void	ProcessKeyInput(const int processedKeyInput, bool pressed) = 0;
	virtual void	ProcessMouseInput(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags, bool pressed) = 0;

	virtual bool	IsKeyPressed(const int key) const = 0;
};

extern IInputSystem* gpInputSystem;
