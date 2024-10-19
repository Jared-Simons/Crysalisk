#pragma once

#include "defines.h"

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_VERBOSE = 5
} log_level;

b8 logging_initialize();
void logging_shutdown();

void log_message(log_level message_severity, const char* message, ...);
