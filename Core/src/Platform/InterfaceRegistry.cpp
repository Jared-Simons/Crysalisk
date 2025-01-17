#include "InterfaceRegistry.h"

#include "Strings/String_Utils.h"
#include <malloc.h> // THIS IS TEMPORARY UNTIL WE HAVE ALL MEMORY ALLOCATING FROM A SHARED ALLOCATOR

namespace Platform {

	static Utl::CVector<SystemInterface_t> registeredSystems{};

	void RegisterInterface(SystemInterface_t& interfaceData)
	{
		registeredSystems.PushBack(interfaceData);
	}

	Utl::CVector<SystemInterface_t>& GetRegisteredInterfaces()
	{
		return registeredSystems;
	}

	void CleanupInterfaceRegistry()
	{
		// TODO: Cleanup the strings if necessary...
	}

}