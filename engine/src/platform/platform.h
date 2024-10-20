#pragma once

#include "defines.h"

typedef struct platform_state_config {
    const char* application_name;
} platform_state_config;

typedef struct window {
    const char* window_title;
    void* internal_state;
} window;

typedef struct platform_state {
    window main_window;
    void* internal_state;
} platform_state;

b8 platform_initialize(u64* memory_requirement, void* state, platform_state_config config);
b8 platform_update();

b8 platform_create_window(const char* window_title, window* out_window);

b8 platform_process_messages(window* window);
