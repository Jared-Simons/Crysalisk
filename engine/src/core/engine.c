#include "engine.h"

#include "core/logging.h"

// Engine state
engine_state_t* state_ptr = 0;

b8 engine_initialize(struct engine_state_t* engine_state) {
    if (!engine_state) {
        // TODO: Logging
        return false;
    }

    state_ptr = engine_state;

    logging_initialize();

    LOG_INFO("%s initialized!", engine_state->application_name);

    LOG_INFO("Logger test, %.5f", 3.14159);

    LOG_FATAL("FATAL TEST, %d", 400);
    LOG_ERROR("ERROR TEST, %d", 350);
    LOG_WARNING("WARNING TEST, %d", 300);
    LOG_INFO("INFO TEST, %d", 250);
    LOG_DEBUG("DEBUG TEST, %d", 200);
    LOG_TRACE("TRACE TEST, %d", 150);

    C_ASSERT_MSG(1 == 0, "One does not equal zero!");

    return true;
}

b8 engine_run(struct engine_state_t* engine_state) {
    return true;
}

void engine_shutdown(struct engine_state_t* engine_state) {
    log_message(LOG_LEVEL_INFO, "Engine shutting down");

    logging_shutdown();
}
