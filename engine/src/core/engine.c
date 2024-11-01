#include "engine.h"

#include "core/logging.h"
#include "core/memory.h"
#include "defines.h"
#include "platform/platform.h"

#include "containers/darray.h"

// Engine state
static engine_state_t* state_ptr = 0;

b8 engine_initialize(struct engine_state_t* engine_state) {
    if (!engine_state) {
        // TODO: Logging
        return false;
    }

    state_ptr = engine_state;

    if (!memory_initialize()) {
        LOG_ERROR("Memory system failed to initialize! Application cannot continue.");
        return false;
    }

    logging_initialize();

    // Platform layer.
    platform_state_config platform_config = {};
    platform_config.application_name = engine_state->application_name;
    platform_initialize(&engine_state->platform_memory_requirement, 0, platform_config);
    engine_state->platform_state = memory_allocate(engine_state->platform_memory_requirement, MEMORY_TAG_ENGINE);
    if (!platform_initialize(&engine_state->platform_memory_requirement, engine_state->platform_state, platform_config)) {
        LOG_ERROR("Failed to initialize platform layer! Application cannot continue.");
        return false;
    }

    LOG_INFO("%s initialized!", engine_state->application_name);

    LOG_INFO("Logger test, %.5f", 3.14159);

    LOG_FATAL("FATAL TEST, %d", 400);
    LOG_ERROR("ERROR TEST, %d", 350);
    LOG_WARNING("WARNING TEST, %d", 300);
    LOG_INFO("INFO TEST, %d", 250);
    LOG_DEBUG("DEBUG TEST, %d", 200);
    LOG_TRACE("TRACE TEST, %d", 150);

    // C_ASSERT_MSG(1 == 0, "One does not equal zero!");

    return true;
}

b8 engine_run(struct engine_state_t* engine_state) {
    if (!platform_update()) {
        return false;
    }

    return true;
}

void engine_shutdown(struct engine_state_t* engine_state) {
    log_message(LOG_LEVEL_INFO, "Engine shutting down");

    memory_free(engine_state->platform_state, engine_state->platform_memory_requirement, MEMORY_TAG_ENGINE);

    logging_shutdown();

    memory_shutdown();
}
