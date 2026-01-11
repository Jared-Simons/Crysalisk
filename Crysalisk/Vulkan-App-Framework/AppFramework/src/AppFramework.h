//=============================================================================
// Author: Jared Simons
// Date Created: 1/11/2026
// 
// Purpose: Interface for the Application Framework.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Core/Engine.h>

namespace AppFramework
{
	struct AppFrameworkConfig_t
	{
		class ::IEngine* pEngine{ nullptr };
	};

	class IAppFramework
	{
	public:
		virtual bool	Initialize(AppFrameworkConfig_t& pConfig) = 0;
	};

	extern IAppFramework* g_pAppFramework;

} // namespace AppFramework
