//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#pragma once

#include <Defines.h>

struct JobSystemConfig_t
{
	size_t	iNumJobThreads{ 0 };
};

class IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) = 0;
	virtual void	Shutdown() = 0;
};

extern IJobSystem* g_pJobSystem;
