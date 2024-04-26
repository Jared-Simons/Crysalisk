//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Provides logging utilities to be used throughout the codebase.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

const int MAX_LOGGERS = 1;

bool Logging_Initialize();
void Logging_Shutdown();

enum class eLogLevel
{
	LOG_LEVEL_VERBOSE = 0,
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_DEBUG = 2,
	LOG_LEVEL_WARN = 3,
	LOG_LEVEL_ERROR = 4,
	LOG_LEVEL_FATAL = 5
};

class ENGINE_API Logger
{
public:
	Logger(const char* name);

	void LogMessage(eLogLevel level, const char* message, ...);

	void SetDebugName(const char* name) { m_loggerName = name; }

private:
	const char* m_loggerName{ nullptr };
	int m_loggerIndex{ -1 };
};

class LogSystem
{
public:
	bool	InitLoggingSystems();

	int		RegisterLogger(Logger& logger);

	// TODO: Check for nullptrs
	Logger* GetLoggerByIndex(int index) const;

private:
	Logger* m_RegiseredLoggers[MAX_LOGGERS];
};

// General-purpose logging macros
// These macros assume that there will always be a general logging channel at index 0, not very flexible...
#define LOG_VERBOSE(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_VERBOSE, message, __VA_ARGS__))
#define LOG_INFO(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_INFO, message, __VA_ARGS__))
#define LOG_DEBUG(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_DEBUG, message, __VA_ARGS__))
#define LOG_WARN(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_WARN, message, __VA_ARGS__))
#define LOG_ERROR(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_ERROR, message, __VA_ARGS__))
#define LOG_FATAL(message, ...) (g_Logger->GetLoggerByIndex(0)->LogMessage(eLogLevel::LOG_LEVEL_FATAL, message, __VA_ARGS__))

extern LogSystem* g_Logger;