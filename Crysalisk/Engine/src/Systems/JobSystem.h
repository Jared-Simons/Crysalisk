//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#pragma once

#include <Defines.h>

typedef int (*PFN_Job_Entry_Point)(void* pParams);

enum class eJobPriority
{
	LOW,
	NORMAL,
	HIGH,
};

struct JobDeclaration_t
{
	PFN_Job_Entry_Point	pfnEntryPoint{ nullptr };
	void*				pParams{ nullptr };
	eJobPriority		ePriority{ eJobPriority::NORMAL };
};

struct JobSystemConfig_t
{
	size_t	iNumJobThreads{ 0 };
};

class IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) = 0;
	virtual void	Shutdown() = 0;

	virtual bool	AddJob(JobDeclaration_t& pJobInfo) = 0;
};

extern IJobSystem* g_pJobSystem;
