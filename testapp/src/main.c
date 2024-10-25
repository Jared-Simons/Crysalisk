#include <defines.h>

#include <core/engine.h>

#include <stdlib.h> // TODO: Temp

i32 main() {
    engine_state_t* engine_state = (struct engine_state_t*)malloc(sizeof(engine_state_t));

    engine_state->application_name = "Crysalisk Engine";
    if (!engine_initialize(engine_state)) {
    }

    while (true) {
        engine_run(engine_state);
    }

    engine_shutdown(engine_state);

    free(engine_state);

    return 0;
}
