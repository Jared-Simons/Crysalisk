//=============================================================================
// Author: Jared Simons
// Date Created: 2/29/2024
// 
// Purpose: Provides logging utilities to be used throughout the codebase.
//=============================================================================
#pragma once

#include "Defines.h"
#include "Platform/Platform.h"

enum class eLogLevel
{
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_DEBUG = 4,
	LOG_LEVEL_VERBOSE = 5
};

//-------------------------------------------------------------------------
// The Logging System interface.
//-------------------------------------------------------------------------
class CORE_API LogSystem
{
public:
	bool	Init(i32 iLogVerbosity);
	void	Shutdown();

	void	LogMessage(eLogLevel level, const char* message, ...);

	// Use this to register a console as a listener to receive messages to display.
	void	RegisterConsoleWriteCallback(void* pConsole, void (*PFN_ConsoleEvent_MessageSent)(void* pConsole, const char* pszMessage));
	void	UnregisterConsoleWriteCallback(void* pConsole);

private:
	static i32 m_LogVerbosity;
};

// Logging system context.
CORE_API extern LogSystem* g_pLog;

// General-purpose logging macros
#define LOG_FATAL(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_FATAL, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_ERROR, message, __VA_ARGS__)
#define LOG_WARN(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_WARN, message, __VA_ARGS__)
#define LOG_INFO(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_INFO, message, __VA_ARGS__)
#define LOG_DEBUG(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_DEBUG, message, __VA_ARGS__)
#define LOG_VERBOSE(message, ...) g_pLog->LogMessage(eLogLevel::LOG_LEVEL_VERBOSE, message, __VA_ARGS__)