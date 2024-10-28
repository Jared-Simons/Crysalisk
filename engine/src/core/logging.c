#include "logging.h"

#include "platform/platform.h"

#include <stdarg.h>
#include <stdio.h> // TODO: temp

const char* log_level_strings[LOG_LEVEL_MAX_LEVELS] = {
    "[FATAL]\t ",
    "[ERROR]\t ",
    "[WARN]\t ",
    "[INFO]\t ",
    "[DEBUG]\t ",
    "[VERBOSE]",
};

b8 logging_initialize() {
    // TODO: nothing to do for now.
    return true;
}

void logging_shutdown() {
    // TODO: nothing to do for now.
}

void log_message(log_level message_severity, const char* message, ...) {
    char message_buffer[32000];
    va_list var_args;
    va_start(var_args, message);
    vsnprintf(message_buffer, 32000, message, var_args);
    va_end(var_args);

    char message_buffer2[32000];
    sprintf(message_buffer2, "%s%s\n", log_level_strings[message_severity], message_buffer);
    // Send to the platform layer for output.
    platform_log_message(message_severity, message_buffer2);
}

void log_assertion_failure(const char* message, const char* filename, i32 line) {
    char message_buffer[32000];
    sprintf(message_buffer, "Assertion failure! '%s' (File: '%s', Line: %d)", message, filename, line);

    LOG_ERROR(message_buffer);
}
