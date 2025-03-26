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

	struct SystemInterface_t
	{
		void* pSystemAddr;			// A pointer to the system's interface.
		char* szSystemName;			// The name of the system.
	};

	CORE_API void RegisterInterface(SystemInterface_t& interfaceData);
	CORE_API Utl::CVector<SystemInterface_t>& GetRegisteredInterfaces();

	void CleanupInterfaceRegistry();

}