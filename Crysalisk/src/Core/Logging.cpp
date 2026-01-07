//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Implements logging utilities to be used throughout the codebase.
//=============================================================================
#include "Logging.h"

#include "Platform/FileSystem.h"

#include <stdarg.h>
#include <stdio.h>

// Logging System context.
static LogSystem s_Log;
LogSystem* g_pLog = &s_Log;

int LogSystem::m_LogVerbosity = 6;

// File to write logging entries out to.
FileHandle_t s_LoggerFile{};

// NOTE: This MUST stay aligned with the eLogLevel enum
static const char* logLevelStrings[6] =
{
	"[FATAL]\t ",
	"[ERROR]\t ",
	"[WARN]\t ",
	"[INFO]\t ",
	"[DEBUG]\t ",
	"[VERBOSE]",
};

//-------------------------------------------------------------------------
// Purpose: Initializes the logging system.
// Input - none.
// Output - bool: Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool LogSystem::Init(int iLogVerbosity)
{
	m_LogVerbosity = iLogVerbosity;

	// Set up our logging file.
	if (!FileSystem::OpenFile("console.log", FILEMODE_WRITE, false, &s_LoggerFile))
	{
		LOG_WARN("LogSystem::Init() -> Failed to create/open file to write logging messages to. Logging entries will not be saved.");
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the logging system.
//-------------------------------------------------------------------------
void LogSystem::Shutdown()
{
	// Close our logging file.
	FileSystem::CloseFile(&s_LoggerFile);
}

//-------------------------------------------------------------------------
// Purpose: The main logging method.
// Input - level: The message's severity level.
//		   message: The message/format string.
//		   var_args: arguments to format the message string, similar in style to printf.
// Output - none.
//-------------------------------------------------------------------------
void LogSystem::LogMessage(eLogLevel level, const char* message, ...)
{
	int logLevel = static_cast<int>(level);

	if (logLevel > LogSystem::m_LogVerbosity || logLevel < 0) // Just checking for an invalid index...
	{
		// Logging for these messages is disabled, just return.
		return;
	}

	char tempMessageBuffer[4092];
	char formattedMessageBuffer[4092];

	sprintf_s(tempMessageBuffer, "%s\0", message);

	va_list va_args;
	va_start(va_args, message);
	vsprintf_s(formattedMessageBuffer, tempMessageBuffer, va_args);
	va_end(va_args);

	// Prepend channel string to the message
	char finalOutBuffer[4092];
	sprintf_s(finalOutBuffer, "%s%s\n", logLevelStrings[logLevel], formattedMessageBuffer);

	// Write the message out to file, if possible.
	if (s_LoggerFile.bIsValid)
	{
		FileSystem::WriteLine(finalOutBuffer, &s_LoggerFile);
	}

	Platform::LogMessage(logLevel, finalOutBuffer);
}
