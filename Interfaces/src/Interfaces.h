//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2025
// 
// Purpose: This static library provides interfaces to shared engine systems.
// 
// Lets say that I am a library or application that wants to have access to some of the engine's shared interfaces
// (or I want to provide one for other's to use). Whatever should I do?
// 
// You can link your code against this library and include this file to get access to all engine interfaces.
// You can also declare one in this file to provide it to anyone else who links with this.
// 
// Any library that links with this MUST implement the 'ReturnInterfacesToRegistry()' function.
// 
// The process for gaining access to interfaces is as follows:
// 
// 1.) Some code loads your library, the code responsible for loading your library lives in the Core.dll, in the platform layer.
// 2.) The library loading code will search for an exported method 'Utl::CVector<SystemInterface_t> ObtainInterfaces()'
//			This method is responsible for getting the addresses (and names) off all system interface instances, and returning them to the interface registry.
// 3.) Once every library has provided their interfaces to the registry, the engine will call 'void AssignInterfaces()', defined below. 
//		This will populate your library's unique pointers to the global interface for each system.
//		YOUR CODE IS RESPONSIBLE FOR CHECKING THAT ALL YOUR REQUIRED INTERFACES ARE PRESENT, AND, IF NOT, HOW TO HANDLE ERROR CONDITIONS.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Platform/InterfaceRegistry.h>

#include <Strings/String_Utils.h>

// Engine System Interfaces
static class IEventSystem* g_pEngineEvents{ nullptr };
static class IInputSystem* g_pInputSystem{ nullptr };
static class IMaterialSystem* g_pMaterialSystem{ nullptr };

//-------------------------------------------------------------------------
// Purpose: A function to be implemented by the consuming library.
//			This is supposed to call Platform::RegisterInterface() on all
//			of it's shared interfaces so they can be propagated out to all other libs.
// 
// Input: none.
// Output: bool - Returns true on successfully returned interfaces.
//-------------------------------------------------------------------------
bool ReturnInterfacesToRegistry();

//-------------------------------------------------------------------------
// Purpose: This function will go through and populate all of the above
// pointers-to-interfaces so that the consuming library can access these systems
// with ease. This gets called after all registered libraries have provided their
// interfaces through 'ReturnInterfacesToRegistry()'.
// 
// Input: appInterfaces - A dynamic array containing addresses and names of the ]
// various subsystems provided by engine and other libraries.
// Output: bool - none.
//-------------------------------------------------------------------------
void AssignInterfaces(Utl::CVector<Platform::SystemInterface_t>& appInterfaces)
{
	for (int i = 0; i < appInterfaces.Count(); i++)
	{
		if (StringCompare(appInterfaces[i].szSystemName, "engine_events"))
		{
			g_pEngineEvents = (IEventSystem*)appInterfaces[i].pSystemAddr;
			continue;
		}

		if (StringCompare(appInterfaces[i].szSystemName, "input_system"))
		{
			g_pInputSystem = (IInputSystem*)appInterfaces[i].pSystemAddr;
			continue;
		}

		if (StringCompare(appInterfaces[i].szSystemName, "material_system"))
		{
			g_pMaterialSystem = (IMaterialSystem*)appInterfaces[i].pSystemAddr;
			continue;
		}
	}

	if (g_pEngineEvents != nullptr && g_pInputSystem != nullptr)
	{
		LOG_INFO("'%s' - Interfaces connected!", DLLNAME);
	}
}