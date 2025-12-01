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

enum eWorkerThreadType
{
	WORKER_THREAD_TYPE_GENERAL = 0x1,
	WORKER_THREAD_TYPE_DISK_LOADING = 0x2
};

struct JobDeclaration_t
{
	PFN_Job_Entry_Point	pfnEntryPoint{ nullptr };
	void*				pParams{ nullptr };
	size_t				iParamSize{ 0 };
	uint32_t			iJobType{ WORKER_THREAD_TYPE_GENERAL };
	eJobPriority		ePriority{ eJobPriority::NORMAL };

	void ResetEntry()
	{
		pfnEntryPoint = nullptr;
		iJobType = WORKER_THREAD_TYPE_GENERAL;
		pParams = nullptr;
		ePriority = eJobPriority::NORMAL;
	}
};

struct JobSystemConfig_t
{
	size_t		iNumJobThreads{ 0 };
	uint32_t	iWorkerThreadTypes[15];
};

class IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) = 0;
	virtual void	Shutdown() = 0;
	virtual void	Update() = 0;

	virtual bool	SubmitJob(JobDeclaration_t& pJobInfo) = 0;
};

extern IJobSystem* g_pJobSystem;
