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

Logger::Logger(const char* name)
	: m_loggerName(name)
{
	m_loggerIndex = g_Logger->RegisterLogger(*this);

	if (m_loggerIndex != -1)
		printf("Logger '%s' initialized!\n", m_loggerName);
	else
		printf("Logger '%s' could not be registered!\n", m_loggerName);
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

	// Append channel string to the message
	char outMessage2[4092];
	sprintf(outMessage2, "[%s]\t%s\n", m_loggerName, outMessage);

	// TODO: Platform-specific logging
	printf("%s", outMessage2);
}

LogSystem::LogSystem()
{
	for (int i = 0; i < MAX_LOGGERS; i++)
	{
		m_RegiseredLoggers[i] = nullptr;
	}

	// Create a general use logger.
	m_RegiseredLoggers[0] = new Logger("General");
}

int LogSystem::RegisterLogger(Logger& logger)
{
	int loggerIndex = -1;

	for (int i = 0; i < MAX_LOGGERS; i++)
	{
		if (m_RegiseredLoggers[i] == nullptr)
		{
			loggerIndex = i;
			m_RegiseredLoggers[i] = &logger;
			break;
		}
	}

	return loggerIndex;
}

// Global logging interface
static LogSystem s_Logger;
LogSystem* g_Logger = &s_Logger;