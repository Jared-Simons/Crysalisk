#include "logging.h"

#include <stdio.h> // TODO: temp

b8 logging_initialize() {
    // TODO: nothing to do for now.
    return true;
}

void logging_shutdown() {
    // TODO: nothing to do for now.
}

void log_message(log_level message_severity, const char* message, ...) {
    printf("%s\n", message);
}
