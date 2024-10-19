#pragma once

#include "engine_types.h"

DLL_API b8 engine_initialize(struct engine_state_t* engine_state);
DLL_API b8 engine_run(struct engine_state_t* engine_state);
DLL_API void engine_shutdown(struct engine_state_t* engine_state);
