#pragma once

#include "defines.h"

typedef struct engine_state_t {
    char* application_name;

    u64 platform_memory_requirement;
    void* platform_state;
} engine_state_t;
