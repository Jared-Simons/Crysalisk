//=============================================================================
// Author: Jared Simons
// Date Created: 7/9/2026
// 
// Purpose: Console variables.
//=============================================================================
#include "ConVar.h"

// Core
#include "Assertions.h"
#include "Containers/CVector.h"
#include "Strings/String_Utils.h"

struct ConVarSystemState
{
	Utl::CVector<ConVar_t>	registeredCommands{};
};

void AllocateConVarSystemState(struct ConVarSystemState** ppSystemState)
{
	*ppSystemState = new ConVarSystemState;
}

bool InitializeConVarSystem(struct ConVarSystemState* pSystem)
{
	return true;
}

void ShutdownConVarSystem(struct ConVarSystemState* pSystem)
{
	pSystem->registeredCommands.Clear();
}

bool ConsoleCommandRegister(struct ConVarSystemState* pSystem, char* commandName, eConVar_ValueType valueType, PFN_ConVar_Callback callback, void* listener)
{
	// FIXME: Handle duplicate command names?
	CASSERTMSG(commandName && callback, "ConsoleCommandRegister() - requires a name and callback for the console command.");

	pSystem->registeredCommands.PushBack(ConVar_t{});
	ConVar_t* pNewVar = &pSystem->registeredCommands.Back();

	pNewVar->name = Utl::StringDuplicate(commandName);
	pNewVar->dataType = valueType;
	Platform::MemorySet(&pNewVar->data, 0, sizeof(pNewVar->data));
	pNewVar->callback = callback;
	pNewVar->listenerInst = listener;

	return true;
}

void ConsoleCommandExecute(struct ConVarSystemState* pSystem, char* pCommandString)
{
	CASSERTMSG(pSystem && pCommandString, "ConsoleCommandExecute - requires a pointer to the system state and command name.");

	// Split string into command name and parameters.
	u32 spaceIndex = Utl::StringIndexOf(pCommandString, ' ');
	char* pszCommandName = nullptr;
	char* pszParameters = nullptr;

	if (spaceIndex != kInvalidID_U32)
	{
		pszCommandName = Utl::StringDuplicate_N(pCommandString, spaceIndex);
		pszParameters = Utl::StringTrim(&pCommandString[spaceIndex]);
	}
	else
	{
		// No parameters? Whole string is a command name.
		pszCommandName = Utl::StringDuplicate(pCommandString);
	}

	bool foundCommand = false;
	ConVar_t* pCurCommand = nullptr;
	for (u32 iCmd = 0; iCmd < pSystem->registeredCommands.Count(); iCmd++)
	{
		pCurCommand = &pSystem->registeredCommands[iCmd];
		if (Utl::StringCompare(pCurCommand->name, pszCommandName))
		{
			// Found it.

			// If parameters were provided, set the value.
			if (pszParameters)
			{
				switch (pCurCommand->dataType)
				{
				case CONVAR_VALUETYPE_INT:
					pCurCommand->data.i64 = Utl::String_ATOI(pszParameters);
					break;
				case CONVAR_VALUETYPE_STRING:
					pCurCommand->data.str = Utl::StringDuplicate(pszParameters);
					break;
				case CONVAR_VALUETYPE_PASSTHROUGH:
					// Just passing through.
					break;
				default:
					CAssert(0);
				}
			}

			// Execute the callback.
			if (pCurCommand->dataType == CONVAR_VALUETYPE_PASSTHROUGH)
			{
				// Just use the raw data, it will be up to the command callback to decipher it.
				pCurCommand->callback(pCurCommand, pszParameters);
			}
			else
			{
				pCurCommand->callback(pCurCommand, &pCurCommand->data);
			}
			foundCommand = true;
		}
	}

	if (!foundCommand)
	{
		LOG_ERROR("Command '%s' does not exist.", pCommandString);
	}

	// Release strings
	if (pszCommandName)
	{
		Utl::StringFree(pszCommandName);
		pszCommandName = nullptr;
	}

	if (pszParameters)
	{
		Utl::StringFree(pszParameters);
		pszParameters = nullptr;
	}
}

void ConsoleCommandSet_Int(struct ConVarSystemState* pSystem, char* commandName, i64 value)
{
	CAssert(pSystem && commandName);

	bool foundCommand = false;
	ConVar_t* pCurCommand = nullptr;
	for (u32 iCmd = 0; iCmd < pSystem->registeredCommands.Count(); iCmd++)
	{
		pCurCommand = &pSystem->registeredCommands[iCmd];
		if (Utl::StringCompare(pCurCommand->name, commandName))
		{
			// Found it.
			pCurCommand->data.i64 = value;
			foundCommand = true;
		}
	}

	if (!foundCommand)
	{
		LOG_ERROR("Command '%s' does not exist.", commandName);
	}
}