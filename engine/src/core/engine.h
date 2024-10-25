#pragma once

#include "defines.h"

typedef struct engine_state_t {
    char* application_name;

    u64 platform_memory_requirement;
    void* platform_state;
} engine_state_t;

DLL_API b8 engine_initialize(struct engine_state_t* engine_state);
DLL_API b8 engine_run(struct engine_state_t* engine_state);
DLL_API void engine_shutdown(struct engine_state_t* engine_state);
