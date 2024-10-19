#include "engine.h"

#include "core/logging.h"

#include <stdio.h>

b8 engine_initialize(struct engine_state_t* engine_state) {
    if (!engine_state) {
        // TODO: Logging
        return false;
    }

    logging_initialize();

    printf("%s initialized!\n", engine_state->application_name);

    log_message(LOG_LEVEL_INFO, "Logger test");

    return true;
}

b8 engine_run(struct engine_state_t* engine_state) {
    return true;
}

void engine_shutdown(struct engine_state_t* engine_state) {
    log_message(LOG_LEVEL_INFO, "Engine shutting down");

    logging_shutdown();
}
