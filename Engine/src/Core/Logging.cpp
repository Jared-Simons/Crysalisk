//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Implements logging utilities to be used throughout the codebase.
//=============================================================================
#include "Logging.h"

#include <stdarg.h>
#include <stdio.h>

// NOTE: This MUST stay aligned with the eLogLevel enum
const char* logLevelStrings[6] =
{
	"[VERBOSE]",
	"[INFO]\t ",
	"[DEBUG]\t ",
	"[WARN]\t ",
	"[ERROR]\t ",
	"[FATAL]\t "
};

bool LoggingInitialize()
{
	return true;
}

void Logger::LogMessage(eLogLevel level, const char* message, ...)
{
	char messageBuffer[4092];
	char outMessage[4092];

	sprintf(messageBuffer, "%s\0", message);

	va_list va_args;
	va_start(va_args, message);
	vsprintf(outMessage, messageBuffer, va_args);
	va_end(va_args);

	// Append log level string to the message
	char outMessage2[4092];
	sprintf(outMessage2, "%s%s\n", logLevelStrings[(int)level], outMessage);

	// TODO: Platform-specific logging
	printf("%s", outMessage2);
}