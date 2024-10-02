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
	"[FATAL]\t ",
	"[ERROR]\t ",
	"[WARN]\t ",
	"[INFO]\t ",
	"[DEBUG]\t ",
	"[VERBOSE]",
};

int LogSystem::m_LogVerbosity = 6;

bool LogSystem::Init(int iLogVerbosity)
{
	m_LogVerbosity = iLogVerbosity;

	return true;
}

void LogSystem::Shutdown()
{
}

void LogSystem::LogMessage(eLogLevel level, const char* message, ...)
{
	if (static_cast<int>(level) > LogSystem::m_LogVerbosity)
	{
		// Logging for these messages is disabled, just return.
		return;
	}

	char messageBuffer[4092];
	char outMessage[4092];

	sprintf_s(messageBuffer, "%s\0", message);

	va_list va_args;
	va_start(va_args, message);
	vsprintf_s(outMessage, messageBuffer, va_args);
	va_end(va_args);

	// Prepend channel string to the message
	char outMessage2[4092];
	sprintf_s(outMessage2, "%s%s\n", logLevelStrings[(int)level], outMessage);

	Platform::LogMessage(static_cast<int>(level), outMessage2);
}