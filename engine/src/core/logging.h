#pragma once

#include "defines.h"

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level;

b8 logging_initialize();
void logging_shutdown();

void log_message(log_level message_severity, const char* message, ...);

// Logs a fatal error message
#define LOG_FATAL(message, ...) log_message(LOG_LEVEL_FATAL, message, __VA_ARGS__)

// Logs an error message
#define LOG_ERROR(message, ...) log_message(LOG_LEVEL_ERROR, message, __VA_ARGS__)

// Logs a warning message
#define LOG_WARNING(message, ...) log_message(LOG_LEVEL_WARN, message, __VA_ARGS__)

// Logs an info level message
#define LOG_INFO(message, ...) log_message(LOG_LEVEL_INFO, message, __VA_ARGS__)

// Logs a debug level message
#define LOG_DEBUG(message, ...) log_message(LOG_LEVEL_DEBUG, message, __VA_ARGS__)

// Logs a trace level message
#define LOG_TRACE(message, ...) log_message(LOG_LEVEL_TRACE, message, __VA_ARGS__)
