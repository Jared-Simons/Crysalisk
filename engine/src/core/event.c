#include "event.h"

#include "containers/darray.h"
#include "core/logging.h"

typedef struct event {
    void* listener_inst;
    PFN_event_callback callback;
} event_t;

typedef struct event_code_events {
    // darray
    event_t* registered_events;
} event_code_events;

typedef struct event_system_state {
    u64 num_registered_events;
    event_code_events events[EVENT_CODE_MAX_CODES];
} event_system_state;

static event_system_state* state_ptr = 0;

b8 event_system_initialize(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(event_system_state);

    if (!state) {
        return true;
    }

    state_ptr = state;

    // Set up our event darrays
    for (int code_index = 0; code_index < EVENT_CODE_MAX_CODES; ++code_index) {
        state_ptr->events[code_index].registered_events = darray_create(K_DARRAY_DEFAULT_CAPACITY, sizeof(event_t));

        u64 num_events = darray_length_get(state_ptr->events[code_index].registered_events);
        for (u32 event_index = 0; event_index < num_events; ++event_index) {
            event_t* current_event = &state_ptr->events[code_index].registered_events[event_index];

            current_event->callback = 0;
            current_event->listener_inst = 0;
        }
    }

    state_ptr->num_registered_events = 0;

    return true;
}

void event_system_shutdown(void* state) {
    if (state) {
        event_system_state* event_sys_state = (event_system_state*)state;

        // Destroy our event darrays
        for (int code_index = 0; code_index < EVENT_CODE_MAX_CODES; ++code_index) {
            darray_destroy(event_sys_state->events[code_index].registered_events);
        }
    }
}

b8 event_system_register(u64 event_code, void* listener_inst, PFN_event_callback PFN_callback) {
    // Get an event to use.
    event_t* event = 0;

    u64 num_events = darray_length_get(state_ptr->events[event_code].registered_events);
    for (u32 event_index = 0; event_index < num_events; ++event_index) {
        if (state_ptr->events[event_code].registered_events[event_index].callback == 0) {
            // We found a free event! Grab it and boot out.
            event = &state_ptr->events[event_code].registered_events[event_index];
            break;
        }
    }

    // If we couldn't find an event, report the failure.
    if (!event) {
        LOG_ERROR("Failed to find free event. Event registration failed.");
        return false;
    }

    // Otherwise, register the event.
    event->callback = PFN_callback;
    event->listener_inst = listener_inst;
    state_ptr->num_registered_events++;

    return true;
}

b8 event_system_unregister(u64 event_code, void* listener_inst, PFN_event_callback PFN_callback) {
    event_t* event = 0;
    b8 event_found = false;

    u64 num_events = darray_length_get(state_ptr->events[event_code].registered_events);
    for (u32 event_index = 0; event_index < num_events; ++event_index) {
        event = &state_ptr->events[event_code].registered_events[event_index];

        if (event->listener_inst == listener_inst && event->callback == PFN_callback) {
            // Found an event, clear and boot out.
            event->listener_inst = 0;
            event->callback = 0;
            state_ptr->num_registered_events--;

            event_found = true;
            break;
        }
    }

    if (!event_found) {
        LOG_ERROR("Failed to unregister event. Nothing was done.");
        return false;
    }

    return true;
}

void event_system_fire(u64 event_code, void* sender, event_data_t event_data) {
    event_t* event = 0;

    u64 num_events = darray_length_get(state_ptr->events[event_code].registered_events);
    for (u32 event_index = 0; event_index < num_events; ++event_index) {
        if (state_ptr->events[event_code].registered_events[event_index].callback) {
            // Fire the event.
            if (state_ptr->events[event_code].registered_events[event_index].callback(event_data)) {
                // If the event returns true, break out. Otherwise, handle further events.
                break;
            }
        }
    }
}
