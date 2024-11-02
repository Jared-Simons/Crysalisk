#include "engine.h"

#include "core/event.h"
#include "core/input.h"
#include "core/logging.h"
#include "core/memory.h"
#include "defines.h"
#include "platform/platform.h"

// Engine state
static engine_state_t* state_ptr = 0;

b8 engine_shutdown_callback(event_data_t event_data);
b8 engine_key_pressed_callback(event_data_t event_data);
b8 engine_key_released_callback(event_data_t event_data);

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

    // Event system
    event_system_initialize(&engine_state->event_system_memory_requirement, 0);
    engine_state->event_system_state = memory_allocate(engine_state->event_system_memory_requirement, MEMORY_TAG_ENGINE);
    if (!event_system_initialize(&engine_state->event_system_memory_requirement, engine_state->event_system_state)) {
        LOG_ERROR("Event system failed to initialize! Application cannot continue.");
        return false;
    }

    // Input system
    input_system_initialize(&engine_state->input_system_memory_requirement, 0);
    engine_state->input_system_state = memory_allocate(engine_state->input_system_memory_requirement, MEMORY_TAG_ENGINE);
    if (!input_system_initialize(&engine_state->input_system_memory_requirement, engine_state->input_system_state)) {
        LOG_ERROR("Input system failed to initialize! Application cannot continue.");
        return false;
    }

    // Platform layer
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

    // Register for the engine shutdown callback.
    event_system_register(EVENT_CODE_APPLICATION_QUIT, 0, engine_shutdown_callback);
    engine_state->application_should_shutdown = false;

    event_system_register(EVENT_CODE_INPUT_KEY_PRESSED, 0, engine_key_pressed_callback);
    event_system_register(EVENT_CODE_INPUT_KEY_RELEASED, 0, engine_key_released_callback);

    return true;
}

b8 engine_run(struct engine_state_t* engine_state) {
    while (engine_state->application_should_shutdown == false) {
        if (!platform_update()) {
            return false;
        }

        input_system_update(engine_state->input_system_state);
    }

    engine_shutdown(engine_state);

    return true;
}

void engine_shutdown(struct engine_state_t* engine_state) {
    log_message(LOG_LEVEL_INFO, "Engine shutting down");

    // TODO: Platform system shutdown.
    memory_free(engine_state->platform_state, engine_state->platform_memory_requirement, MEMORY_TAG_ENGINE);

    event_system_shutdown(engine_state->event_system_state);
    memory_free(engine_state->event_system_state, engine_state->event_system_memory_requirement, MEMORY_TAG_ENGINE);

    logging_shutdown();

    memory_shutdown();
}

// Event callback to shutdown the application.
b8 engine_shutdown_callback(event_data_t event_data) {
    state_ptr->application_should_shutdown = true;
    return true;
}

b8 engine_key_pressed_callback(event_data_t event_data) {
    LOG_DEBUG("Key Pressed: %c", event_data.data.u32[0]);
    return false;
}

b8 engine_key_released_callback(event_data_t event_data) {
    LOG_DEBUG("Key Released: %c", event_data.data.u32[0]);
    return false;
}
