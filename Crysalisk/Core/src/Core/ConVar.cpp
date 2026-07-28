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
				case CONVAR_VALUETYPE_VECTOR:
				{
					Vector temp{};
					bool parsed = Utl::String_ToVector3(pszParameters, &temp);
					if (parsed)
					{
						pCurCommand->data.v[0] = temp.x;
						pCurCommand->data.v[1] = temp.y;
						pCurCommand->data.v[2] = temp.z;
					}
					else
					{
						pCurCommand->data.v[0] = pCurCommand->data.v[1] = pCurCommand->data.v[2] = 0.0f;
						LOG_WARN("Console command '%s' requires an argument of the form f f f, which was not provided. Nothing was done.", pszCommandName);
						goto cleanup_and_exit;
					}
				} break;
				case CONVAR_VALUETYPE_PASSTHROUGH:
					// Just passing through.
					break;
				default:
					CAssert(0);
				}
			}
			else
			{
				// Print the current value of the convar.
				switch (pCurCommand->dataType)
				{
				case CONVAR_VALUETYPE_NONE:
				case CONVAR_VALUETYPE_PASSTHROUGH:
					break;
				case CONVAR_VALUETYPE_INT:
					LOG_INFO("'%s': %d", pCommandString, pCurCommand->data.i64);
					break;
				case CONVAR_VALUETYPE_FLOAT:
					LOG_INFO("'%s': %f", pCommandString, pCurCommand->data.f64);
					break;
				case CONVAR_VALUETYPE_VECTOR:
					LOG_INFO("'%s': %f %f %f", pCommandString, pCurCommand->data.v[0], pCurCommand->data.v[1], pCurCommand->data.v[2]);
					break;
				case CONVAR_VALUETYPE_STRING:
					if (pCurCommand->data.str)
					{
						LOG_INFO("'%s': %s", pCommandString, pCurCommand->data.str);
					}
					break;
				default:
					CAssert(0);
					break;
				}
			}

			// Execute the callback.
			if (pCurCommand->dataType == CONVAR_VALUETYPE_PASSTHROUGH)
			{
				// Just use the raw data, it will be up to the command callback to decipher it.
				pCurCommand->callback(pCurCommand, pszParameters);
			}
			else if (pCurCommand->dataType == CONVAR_VALUETYPE_STRING)
			{
				pCurCommand->callback(pCurCommand, pCurCommand->data.str);
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

cleanup_and_exit:
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