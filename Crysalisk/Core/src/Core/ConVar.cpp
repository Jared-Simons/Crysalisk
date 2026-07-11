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

void ConsoleCommandExecute(struct ConVarSystemState* pSystem, char* commandName)
{
	CASSERTMSG(pSystem && commandName, "ConsoleCommandExecute - requires a pointer to the system state and command name.");

	bool foundCommand = false;
	ConVar_t* pCurCommand = nullptr;
	for (u32 iCmd = 0; iCmd < pSystem->registeredCommands.Count(); iCmd++)
	{
		pCurCommand = &pSystem->registeredCommands[iCmd];
		if (Utl::StringCompare(pCurCommand->name, commandName))
		{
			// Found it.
			pCurCommand->callback(pCurCommand, &pCurCommand->data);
			foundCommand = true;
		}
	}

	if (!foundCommand)
	{
		LOG_ERROR("Command '%s' does not exist.", commandName);
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