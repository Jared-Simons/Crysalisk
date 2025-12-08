//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#pragma once

#include <Defines.h>

typedef int (*PFN_Job_Entry_Point)(void* pParams, void** pJobResults);
typedef int (*PFN_On_Job_Success)(struct JobResults_t& pResults);

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

// THREADTODO: Should this be exposed?
struct JobResults_t
{
	PFN_On_Job_Success	pfnOnJobCompleted_Success_Callback{ nullptr };

	size_t				iUserDataSize{ 0 };
	void* pUserData{ nullptr };
};

struct JobDeclaration_t
{
	PFN_Job_Entry_Point	pfnEntryPoint{ nullptr };
	void*				pParams{ nullptr };
	JobResults_t		pJobResults{};
	size_t				iParamSize{ 0 };
	uint32_t			iJobType{ WORKER_THREAD_TYPE_GENERAL };
	eJobPriority		ePriority{ eJobPriority::NORMAL };

	void ResetEntry()
	{
		pfnEntryPoint = nullptr;
		pParams = nullptr;
		pJobResults.iUserDataSize = 0;
		pJobResults.pUserData = nullptr;
		pJobResults.pfnOnJobCompleted_Success_Callback = nullptr;
		iParamSize = 0;
		iJobType = WORKER_THREAD_TYPE_GENERAL;
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
	virtual void	SubmitJobResults(size_t iResultSize, void* pJobResults) = 0;
};

extern IJobSystem* g_pJobSystem;
