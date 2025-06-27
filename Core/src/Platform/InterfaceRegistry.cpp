//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2025
// 
// Purpose: This file holds opaque pointers to all the engine's systems,
// so that each dll can keep it's own copy with Interfaces.lib.
//=============================================================================
#include "InterfaceRegistry.h"

#include "Strings/String_Utils.h"

namespace Platform 
{
	// An array of all the registered interfaces.
	// This is the 'point-of-truth' for the entire codebase,
	// If an interface pointer changes somewhere, it needs to be updated here to be 'valid'.
	static Utl::CVector<SystemInterface_t> s_RegisteredInterfaces{};

	//-------------------------------------------------------------------------
	// Purpose: Registers the provided interface with the interface registry.
	// Input:  pInterfaceData - The interface to be registered.
	// Output: none.
	//-------------------------------------------------------------------------
	void RegisterInterface(SystemInterface_t& pInterfaceData)
	{
		s_RegisteredInterfaces.PushBack(pInterfaceData);
	}

	//-------------------------------------------------------------------------
	// Purpose: Returns a copy of the registered interfaces.
	// Input:  none.
	// Output: The registered interfaces.
	//-------------------------------------------------------------------------
	Utl::CVector<SystemInterface_t> GetRegisteredInterfaces()
	{
		return s_RegisteredInterfaces;
	}

	//-------------------------------------------------------------------------
	// Purpose: Cleans up the interface registry.
	// Input:  none.
	// Output: none.
	//-------------------------------------------------------------------------
	void CleanupInterfaceRegistry()
	{
		s_RegisteredInterfaces.Clear();
	}
}