//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Implements logging utilities to be used throughout the codebase.
//=============================================================================
#include "Logging.h"

#include "Platform/Platform.h"

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

bool Logging_Initialize()
{
	g_Logger->InitLoggingSystems();

	return true;
}

void Logging_Shutdown()
{
	g_Logger->ShutdownLoggingSystems();
	g_Logger = nullptr;
}

Logger::Logger(const char* name)
	: m_loggerName(name)
{
	m_loggerIndex = g_Logger->RegisterLogger(*this);

	if (m_loggerIndex == -1)
		printf("Logger '%s' could not be registered!\n", m_loggerName);
}

void Logger::LogMessage(eLogLevel level, const char* message, ...)
{
	char messageBuffer[4092];
	char outMessage[4092];

	sprintf_s(messageBuffer, "%s\0", message);

	va_list va_args;
	va_start(va_args, message);
	vsprintf_s(outMessage, messageBuffer, va_args);
	va_end(va_args);

	// Append channel string to the message
	char outMessage2[4092];
	sprintf_s(outMessage2, "%s%s\n", logLevelStrings[(int)level], outMessage);

	Platform::LogMessage((int)level, outMessage2);
}

bool LogSystem::InitLoggingSystems()
{
	for (int i = 0; i < MAX_LOGGERS; i++)
	{
		m_RegiseredLoggers[i] = nullptr;
	}

	// Create a general use logger.
	// Index 0 will always be a general purpose logger.
	m_RegiseredLoggers[0] = new Logger("General");

	return true;
}

void LogSystem::ShutdownLoggingSystems()
{
	delete m_RegiseredLoggers[0];
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

Logger* LogSystem::GetLoggerByIndex(int index) const
{
	// TODO: If this returns nullptr, still print the message, but also error.
	if (m_RegiseredLoggers)
	{
		if (m_RegiseredLoggers[index] != nullptr)
		{
			return m_RegiseredLoggers[index];
		}
		else
		{
			LOG_ERROR("LOGGING ERROR: Invalid Logger Index at %d\n", index);
			return nullptr;
		}
	}
	return nullptr;
}

// Global logging interface
static LogSystem s_Logger;
LogSystem* g_Logger = &s_Logger;