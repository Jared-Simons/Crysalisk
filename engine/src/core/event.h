#pragma once

#include "defines.h"

typedef struct event_data_t {
    union {
        u64 u64[2];
        i64 i64[2];
        f64 f64[2];

        u32 u32[4];
        i32 i32[4];
        f32 f32[4];

        u16 u16[8];
        i16 i16[8];

        u8 u8[16];
        i8 i8[16];

        char str[16];
    } data;
} event_data_t;

typedef b8 (*PFN_event_callback)(event_data_t event_data);

b8 event_system_initialize(u64* memory_requirement, void* state);
void event_system_shutdown(void* state);

b8 event_system_register(u64 event_code, void* listener_inst, PFN_event_callback PFN_callback);
b8 event_system_unregister(u64 event_code, void* listener_inst, PFN_event_callback PFN_callback);
void event_system_fire(u64 event_code, void* sender, event_data_t event_data);

typedef enum engine_event_code {
    EVENT_CODE_UNKNOWN = 0,
    EVENT_CODE_APPLICATION_QUIT,
    EVENT_CODE_WINDOW_RESIZE,
    EVENT_CODE_WINDOW_START_RESIZE,
    EVENT_CODE_WINDOW_END_RESIZE,
    EVENT_CODE_INPUT_KEY_PRESSED,
    EVENT_CODE_INPUT_KEY_RELEASED,
    EVENT_CODE_INPUT_MOUSE_BUTTON_PRESSED,
    EVENT_CODE_INPUT_MOUSE_BUTTON_RELEASED,
    EVENT_CODE_INPUT_MOUSE_MOVED,

    EVENT_CODE_MAX_CODES
} engine_event_code;
