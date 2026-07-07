//=============================================================================
// Author: Jared Simons
// Date Created: 2/20/2025
// 
// Purpose: Some shared input types.
//=============================================================================
#pragma once

//-------------------------------------------------------------------------
// A table of keycodes.
// NOTE: The alphanumeric codes are aligned with their ASCII-counterparts.
//-------------------------------------------------------------------------
enum eKeyTable
{
	KEY_UNDEFINED = -1,

	KEY_0 = 0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,

	KEY_CTRL = 17,

	KEY_BACKSPACE = 23,

	KEY_ESCAPE = 27,

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

//-------------------------------------------------------------------------
// Mouse Button Table
//-------------------------------------------------------------------------
enum eMouseButtonTable
{
	MOUSE_BUTTON_UNDEFINED = 0,

	MOUSE_BUTTON_LEFT = 0x1,
	MOUSE_BUTTON_RIGHT = 0x2,
	MOUSE_BUTTON_MIDDLE = 0x4,

	MAX_MOUSE_BUTTONS
};

//-------------------------------------------------------------------------
// String representations of the key table above.
//-------------------------------------------------------------------------
// NOTE!!!: This string array MUST stay in parody with e_KeyTable!!
static const char* e_KeyTable_Strings[MAX_INPUT_KEYS] = {
	"UNDEFINED KEY!",

	"CTRL",

	"ESC",

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

//-------------------------------------------------------------------------
// String representation of the mouse button table above.
//-------------------------------------------------------------------------
static const char* e_MouseButtonTable_Strings[MAX_MOUSE_BUTTONS] = {
	"UNDEFINED",

	"LEFT",
	"RIGHT",
	"MIDDLE",
};
