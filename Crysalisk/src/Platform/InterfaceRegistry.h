//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2025
// 
// Purpose: This file holds opaque pointers to all the engine's systems,
// so that each dll can keep it's own copy with Interfaces.lib.
//=============================================================================
#pragma once

#include "Containers/CVector.h"

namespace Platform 
{
	//-------------------------------------------------------------------------
	// Contains details about interfaces to be registered.
	//-------------------------------------------------------------------------
	struct SystemInterface_t
	{
		void* pSystemAddr;			// A pointer to the system's interface.
		char* szSystemName;			// The name of the system.
	};

	// Registers the provided interface with the interface registry.
	CORE_API void RegisterInterface(SystemInterface_t& pInterfaceData);

	// Returns a copy of the registered interfaces.
	CORE_API Utl::CVector<SystemInterface_t> GetRegisteredInterfaces();

	// Cleans up the interface registry.
	CORE_API void CleanupInterfaceRegistry();
}