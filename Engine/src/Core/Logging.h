//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Provides logging utilities to be used throughout the codebase.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

bool LoggingInitialize();
void LoggingShutdown();

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
	Logger(const char* name)
		: m_debugName(name)
	{}

	static void LogMessage(eLogLevel level, const char* message, ...);

	void SetDebugName(const char* name)
	{
		m_debugName = name;
	}

private:
	const char* m_debugName;
};

#define LOG_VERBOSE(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_VERBOSE, message, __VA_ARGS__))
#define LOG_INFO(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_INFO, message, __VA_ARGS__))
#define LOG_DEBUG(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_DEBUG, message, __VA_ARGS__))
#define LOG_WARN(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_WARN, message, __VA_ARGS__))
#define LOG_ERROR(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_ERROR, message, __VA_ARGS__))
#define LOG_FATAL(message, ...) (Logger::LogMessage(eLogLevel::LOG_LEVEL_FATAL, message, __VA_ARGS__))