//=============================================================================
// Author: Jared Simons
// Date Created: 7/9/2026
// 
// Purpose: Console variables.
//=============================================================================
#pragma once

#include "Defines.h"

// Callback for console commands.
typedef void (*PFN_ConVar_Callback)(struct ConVar_t* pCommand, void* pValue);

enum eConVar_ValueType
{
	CONVAR_VALUETYPE_NONE = 0,
	CONVAR_VALUETYPE_INT,
	CONVAR_VALUETYPE_FLOAT,
	CONVAR_VALUETYPE_VECTOR,
	CONVAR_VALUETYPE_STRING,

	// A special value type used for commands that accept data as parameters, but do not store them. The callback function will need to interpret the data.
	CONVAR_VALUETYPE_PASSTHROUGH,	
};

struct ConVar_t
{
	char* name{ nullptr };
	u8 dataType{ CONVAR_VALUETYPE_NONE };
	union commandData
	{
		u64 u64;
		i64 i64;
		f64 f64;
		bool b;
		f32 v[3];
		char* str;
	} data;
	PFN_ConVar_Callback callback{ nullptr };
	void* listenerInst{ nullptr };
};

CORE_API void AllocateConVarSystemState(struct ConVarSystemState** ppSystemState);
CORE_API bool InitializeConVarSystem(struct ConVarSystemState* pSystem);
CORE_API void ShutdownConVarSystem(struct ConVarSystemState* pSystem);

CORE_API bool ConsoleCommandRegister(struct ConVarSystemState* pSystem, char* commandName, eConVar_ValueType valueType, PFN_ConVar_Callback callback, void* listener);
CORE_API void ConsoleCommandExecute(struct ConVarSystemState* pSystem, char* commandString);

// FIXME: ConVar handles?
CORE_API void ConsoleCommandSet_Int(struct ConVarSystemState* pSystem, char* commandName, i64 value);