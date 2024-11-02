#pragma once

#include "defines.h"

typedef enum input_keycodes {
    KEY_A,
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

    INPUT_MAX_KEYS
} input_keycodes;

typedef enum input_mousebuttons {
    MOUSE_BUTTON_LEFT = 0x1,
    MOUSE_BUTTON_RIGHT = 0x2,
    MOUSE_BUTTON_MIDDLE = 0x4,
} input_mousebuttons;

b8 input_system_initialize(u64* memory_requirement, void* state);
void input_system_shutdown(void* state);
void input_system_update();

void input_system_process_key(u32 keycode, b8 pressed);
void input_system_process_mouse_button(u8 mousecode, b8 pressed);
