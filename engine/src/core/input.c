#include "input.h"

#include "core/event.h"
#include "core/memory.h"

// NOTE: The amount to offset ascii characters downwards by during input processing
#define K_ASCII_DIFFERENCE_OFFSET 65

typedef struct key_state_t {
    u8 keycode;
    b8 pressed;
} key_state_t;

typedef struct input_system_state {
    key_state_t keyboard_state[INPUT_MAX_KEYS];
    key_state_t prev_frame_keyboard_state[INPUT_MAX_KEYS];

    u8 mouse_state;
    u8 prev_frame_mouse_state;
} input_system_state;

static input_system_state* state_ptr = 0;

b8 input_system_initialize(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(input_system_state);

    if (!state) {
        return true;
    }

    state_ptr = state;

    // Setup the keyboard state table.
    for (i32 key_index = 0; key_index < INPUT_MAX_KEYS; ++key_index) {
        state_ptr->keyboard_state[key_index].keycode = key_index;
        state_ptr->keyboard_state[key_index].pressed = false;

        state_ptr->prev_frame_keyboard_state[key_index].keycode = key_index;
        state_ptr->prev_frame_keyboard_state[key_index].pressed = false;
    }

    // Setup the mouse state.
    state_ptr->mouse_state = 0;
    state_ptr->prev_frame_mouse_state = 0;

    return true;
}

void input_system_shutdown(void* state) {
}

void input_system_update() {
    if (state_ptr) {
        // Copy the current frame input data to the previous.
        memory_copy(state_ptr->prev_frame_keyboard_state, state_ptr->keyboard_state, sizeof(key_state_t));
        memory_copy(&state_ptr->prev_frame_mouse_state, &state_ptr->prev_frame_mouse_state, sizeof(u8));
    }
}

void input_system_process_key(u32 keycode, b8 pressed) {
    if (state_ptr) {
        i32 key_index = keycode - K_ASCII_DIFFERENCE_OFFSET;
        state_ptr->keyboard_state[key_index].pressed = pressed;

        // Fire off an event.
        u32 event_code = pressed ? EVENT_CODE_INPUT_KEY_PRESSED : EVENT_CODE_INPUT_KEY_RELEASED;
        event_data_t event_data;
        event_data.data.u32[0] = keycode;
        event_system_fire(event_code, 0, event_data);
    }
}

void input_system_process_mouse_button(u8 mousecode, b8 pressed) {
    if (pressed) {
        state_ptr->mouse_state |= mousecode;
    } else {
        state_ptr->mouse_state &= ~mousecode;
    }

    // Fire off an event.
    u32 event_code = pressed ? EVENT_CODE_INPUT_MOUSE_BUTTON_PRESSED : EVENT_CODE_INPUT_MOUSE_BUTTON_RELEASED;
    event_data_t event_data;
    event_data.data.u8[0] = mousecode;
    event_system_fire(event_code, 0, event_data);
}
