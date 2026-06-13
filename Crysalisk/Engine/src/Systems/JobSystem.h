//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#pragma once

#include <Defines.h>

// Signature for the entrypoint of a job.
typedef i32 (*PFN_Job_Entry_Point)(void* pParams, void** pJobResults);
// Signature for a successful/failed job callback.
typedef i32 (*PFN_Job_Completed_Callback)(struct JobResults_t& pResults);

//-------------------------------------------------------------------------
// Job Priorities
//-------------------------------------------------------------------------
enum class eJobPriority
{
	// Highest priority jobs, executed as soon as possible.
	HIGH,
	// Normal priority jobs, executed as soon as high priority jobs are exhausted.
	NORMAL,
	// Low priority jobs, executed once all high and normal priority jobs are finished.
	LOW,
};

//-------------------------------------------------------------------------
// Worker Thread/Job Types
//-------------------------------------------------------------------------
enum eWorkerThreadType
{
	// General use thread.
	WORKER_THREAD_TYPE_GENERAL = 0x1,
	// Indicates that a thread is reserved for resource loading,
	// or that a job will load resources off disk.
	WORKER_THREAD_TYPE_DISK_LOADING = 0x2
};

//-------------------------------------------------------------------------
// Job result data
//-------------------------------------------------------------------------
struct JobResults_t
{
	// The callback to be executed after successful completion of the job.
	// Optional, although required if the job provides user data.
	PFN_Job_Completed_Callback	pfnOnJobCompleted_Success_Callback{ nullptr };

	// The callback to be executed after failure to complete the job.
	// Optional, though required if the job provides user data.
	PFN_Job_Completed_Callback	pfnOnJobCompleted_Failed_Callback{ nullptr };

	// Indicates if the job was successfully completed.
	bool	bWasJobSuccessful{ false };
	// The size of the user data in bytes.
	u64	iUserDataSize{ 0 };
	// An opaque to the job-specific data.
	void*	pUserData{ nullptr };
};

//-------------------------------------------------------------------------
// Job Declarations
//-------------------------------------------------------------------------
struct JobDeclaration_t
{
	// The entry point of the job. Required.
	PFN_Job_Entry_Point	pfnEntryPoint{ nullptr };
	// The size in bytes of the params passed in the pParams pointer.
	u64				iParamSize{ 0 };
	// An opaque pointer to parameters to be passed to the job.
	void*				pParams{ nullptr };
	// Holds the job's results.
	JobResults_t		pJobResults{};
	// The type of job to be executed (corresponds to the eWorkerThreadType enum).
	u32			iJobType{ WORKER_THREAD_TYPE_GENERAL };
	// The job's priority.
	eJobPriority		ePriority{ eJobPriority::NORMAL };

	// Resets a job entry for use later.
	void ResetEntry()
	{
		pfnEntryPoint = nullptr;
		pParams = nullptr;
		pJobResults.iUserDataSize = 0;
		pJobResults.pUserData = nullptr;
		pJobResults.bWasJobSuccessful = false;
		pJobResults.pfnOnJobCompleted_Success_Callback = nullptr;
		pJobResults.pfnOnJobCompleted_Failed_Callback = nullptr;
		iParamSize = 0;
		iJobType = WORKER_THREAD_TYPE_GENERAL;
		ePriority = eJobPriority::NORMAL;
	}
};

//-------------------------------------------------------------------------
// Job System Configuration.
//-------------------------------------------------------------------------
struct JobSystemConfig_t
{
	u64		iNumJobThreads{ 0 };
	u32	iWorkerThreadTypes[15];
};

//-------------------------------------------------------------------------
// Job System Interface.
//-------------------------------------------------------------------------
class IJobSystem
{
public:
	static void		AllocateJobSystemState(IJobSystem** ppSystemState);

	virtual bool	Initialize(JobSystemConfig_t& pAppConfig) = 0;
	virtual void	Shutdown() = 0;
	virtual void	Update() = 0;

	virtual bool	SubmitJob(JobDeclaration_t& pJobInfo) = 0;

	// This will force the calling thread to wait for all registered threads to finish
	// any work they may be doing.
	virtual void	WaitForWorkerThreadsToFinish() const = 0;
};
