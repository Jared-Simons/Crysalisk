//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#include "JobSystem.h"

#include <Containers/CDeque.h>
#include <Containers/CVector.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>

//-------------------------------------------------------------------------
// Worker thread.
//-------------------------------------------------------------------------
struct CWorkerThread
{
	// The unique index of this thread in the job system.
	u8						m_iThreadIdx{ 0 };
	// A pointer to the thread itself.
	Platform::ISystemThread*	m_pThread{};
	// A bitfield corresponding to eWorkerThreadType,
	// describes the types of jobs this thread can do.
	u8						m_iThreadType{ WORKER_THREAD_TYPE_GENERAL };
	// The mutex for this thread's work data.
	Platform::ISystemMutex*		m_pWorkDataMutex{ nullptr };
	// The current job's work data.
	JobDeclaration_t			m_CurrentWorkData{};

	~CWorkerThread()
	{
		if (m_pThread)
		{
			Platform::DestroySystemThread(m_pThread);
			m_pThread = nullptr;
		}

		if (m_pWorkDataMutex)
		{
			Platform::DestroySystemMutex(m_pWorkDataMutex);
			m_pWorkDataMutex = nullptr;
		}
	}
};

//-------------------------------------------------------------------------
// Job System Implementation.
//-------------------------------------------------------------------------
class CJobSystem : public IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pAppConfig) override;
	virtual void	Shutdown() override;
	virtual void	Update() override;

	virtual bool	SubmitJob(JobDeclaration_t& pJobInfo) override;
	virtual void	SubmitJobResults(u64 iResultSize, JobResults_t* pResults);

	virtual void	WaitForWorkerThreadsToFinish() const override;
	virtual void	WaitForWorkerThreadsToShutdown();

private:
	void			ProcessJobQueue(Utl::CDeque<JobDeclaration_t>& pJobQueue);
	void			ProcessJobResults();

private:
	JobSystemConfig_t				m_Config{};
	Platform::ISystemMutex*			m_pJobSystemStateMutex{ nullptr };
	bool							m_bJobSystemActive{ false };

	Utl::CVector<CWorkerThread>		m_pWorkerThreads{};

	Utl::CDeque<JobDeclaration_t>	m_LowPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_NormalPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_HighPriorityJobs{};

	Platform::ISystemMutex*			m_pJobResultMutex{ nullptr };
	Utl::CVector<JobResults_t>		m_JobResults{};

	friend static i32 JobThread_Main(void* pParams);
};

// Job System context.
static CJobSystem* s_JobSystem;

//-------------------------------------------------------------------------
// Purpose: The entrypoint for CWorkerThreads.
//			Will run in a loop forever until either the application terminates
//			or a job becomes available.
// 
// Input:	pParams - An opaque pointer to be cast to a u8 containing the thread's index in the job system.
// Output:	i32 - 0 indicates the thread shutdown successfully, otherwise nonzero.
//-------------------------------------------------------------------------
static i32 JobThread_Main(void* pParams)
{
	u8 iThreadIdx = *(u8*)pParams;
	i32 iThreadID = Platform::GetCurrentThreadID();

	LOG_DEBUG("Spawning job thread ID: %d", iThreadIdx);
	CWorkerThread& pThisThread = s_JobSystem->m_pWorkerThreads[iThreadIdx];

	// We need to lock this so we can see when the application is shutting down.
	s_JobSystem->m_pJobSystemStateMutex->Lock();
	while (s_JobSystem->m_bJobSystemActive)
	{
		s_JobSystem->m_pJobSystemStateMutex->Unlock();
		if (pThisThread.m_CurrentWorkData.pfnEntryPoint)
		{
			JobDeclaration_t& pWorkData = pThisThread.m_CurrentWorkData;
			// Make sure we get a lock on our resources before executing the job.
			pThisThread.m_pWorkDataMutex->Lock();
			if ((pWorkData.pfnEntryPoint)(pWorkData.pParams, &pWorkData.pJobResults.pUserData) == 0)
			{
				pWorkData.pJobResults.bWasJobSuccessful = false;
			}
			else
			{
				pWorkData.pJobResults.bWasJobSuccessful = true;
			}

			if (pWorkData.pJobResults.iUserDataSize > 0 && (!pWorkData.pJobResults.pfnOnJobCompleted_Failed_Callback && !pWorkData.pJobResults.pfnOnJobCompleted_Success_Callback))
			{
				// Result data was provided by the job, but one of the callbacks was not provided. Warn about it.
				LOG_WARN("Job completed with result parameters, but callbacks were not provided to handle them.");
			}

			s_JobSystem->SubmitJobResults(pThisThread.m_CurrentWorkData.pJobResults.iUserDataSize, &pThisThread.m_CurrentWorkData.pJobResults);

			// Clear the job data, so that we don't run it again.
			pThisThread.m_CurrentWorkData.ResetEntry();
			pThisThread.m_pWorkDataMutex->Unlock();
		}
		else
		{
			// Sleep if we have nothing to do.
			Platform::ThreadSleep(100.0);
		}
	}

	s_JobSystem->m_pJobSystemStateMutex->Unlock();
	LOG_WARN("Killing job thread %d", iThreadID);
	pThisThread.m_iThreadIdx = kInvalidID_U8;	// NOTE: This will tell the main thread this we're exiting. NOTE: There's probably a better way to do this.
	return 0;
}

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IJobSystem::AllocateJobSystemState(IJobSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold job system state was not provided! Job system creation failed!");
		return;
	}

	s_JobSystem = new CJobSystem;
	*ppSystemState = s_JobSystem;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CJobSystem::Initialize(JobSystemConfig_t& pAppConfig)
{
	m_Config = pAppConfig;
	m_bJobSystemActive = true;
	m_pJobSystemStateMutex = Platform::CreateSystemMutex();

	// Reasonable defaults.
	m_LowPriorityJobs.Resize(80);
	m_NormalPriorityJobs.Resize(200);
	m_HighPriorityJobs.Resize(80);

	// Make sure we have at least one additional thread (aside from the main thread) to do work on.
	if (pAppConfig.iNumJobThreads < 1)
	{
		LOG_FATAL("One core detected, at least 2 cores are required. Job system initialization failed.");
		return false;
	}
	m_pWorkerThreads.Resize(pAppConfig.iNumJobThreads);
	m_pWorkerThreads.SetCount(pAppConfig.iNumJobThreads);

	for (u8 iThread = 0; iThread < pAppConfig.iNumJobThreads; iThread++)
	{
		m_pWorkerThreads[iThread].m_iThreadIdx = iThread;
		m_pWorkerThreads[iThread].m_pThread = Platform::CreateSystemThread(JobThread_Main, &m_pWorkerThreads[iThread].m_iThreadIdx);
		m_pWorkerThreads[iThread].m_pWorkDataMutex = Platform::CreateSystemMutex();
		m_pWorkerThreads[iThread].m_iThreadType = pAppConfig.iWorkerThreadTypes[iThread];
	}

	// Set up job result data.
	m_pJobResultMutex = Platform::CreateSystemMutex();
	m_JobResults.Resize(50);
	
	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CJobSystem::Shutdown()
{
	// This will kill all job threads.
	// We need to lock this so other threads see the change.
	m_pJobSystemStateMutex->Lock();
	m_bJobSystemActive = false;
	m_pJobSystemStateMutex->Unlock();

	m_JobResults.Clear();
	Platform::DestroySystemMutex(m_pJobResultMutex);
	m_pJobResultMutex = nullptr;

	WaitForWorkerThreadsToShutdown();

	Platform::DestroySystemMutex(m_pJobSystemStateMutex);
	m_pJobSystemStateMutex = nullptr;

	m_pWorkerThreads.Clear();
	m_LowPriorityJobs.Clear();
	m_NormalPriorityJobs.Clear();
	m_HighPriorityJobs.Clear();

	s_JobSystem = nullptr;
}

//-------------------------------------------------------------------------
// Processes the job queues and kicks off any work to be done.
//-------------------------------------------------------------------------
void CJobSystem::Update()
{
	// Process job queues and assign to worker threads.

	// First, process high priority jobs.
	ProcessJobQueue(m_HighPriorityJobs);
	i32 iHighJobHead = m_HighPriorityJobs.HeadIndex();
	
	// Next, the normal priority jobs.
	ProcessJobQueue(m_NormalPriorityJobs);

	// Finally, low priority jobs.
	ProcessJobQueue(m_LowPriorityJobs);

	// After kicking off jobs, check for finished jobs.
	m_pJobResultMutex->Lock();
	ProcessJobResults();
	m_pJobResultMutex->Unlock();
}

//-------------------------------------------------------------------------
// Purpose: Submits a job for async execution.
// Input:	pJobInfo - The details of the job to be queued.
// Output:	bool - True on successful job submission, otherwise false.
//-------------------------------------------------------------------------
bool CJobSystem::SubmitJob(JobDeclaration_t& pJobInfo)
{
	// Make sure there's room for the job in the queues.
	switch (pJobInfo.ePriority)
	{
	case eJobPriority::HIGH:
		if (m_HighPriorityJobs.Count() + 1 > m_HighPriorityJobs.Capacity())
		{
			LOG_ERROR("SubmitJob() -> No more space in the high priority job queue. Job submission failed.");
			return false;
		}
		break;
	case eJobPriority::NORMAL:
		if (m_NormalPriorityJobs.Count() + 1 > m_NormalPriorityJobs.Capacity())
		{
			LOG_ERROR("SubmitJob() -> No more space in the normal priority job queue. Job submission failed.");
			return false;
		}
		break;
	case eJobPriority::LOW:
		if (m_LowPriorityJobs.Count() + 1 > m_LowPriorityJobs.Capacity())
		{
			LOG_ERROR("SubmitJob() -> No more space in the low priority job queue. Job submission failed.");
			return false;
		}
		break;
	}

	// We need an entrypoint to execute!
	if (!pJobInfo.pfnEntryPoint)
	{
		LOG_ERROR("CJobSystem::SubmitJob requires pfnEntryPoint to not be NULL. Nothing was done.");
		return false;
	}

	// Add the job to the appropriate queue.
	switch (pJobInfo.ePriority)
	{
	case eJobPriority::LOW:
		m_LowPriorityJobs.PushBack(pJobInfo);
		break;
	case eJobPriority::NORMAL:
		m_NormalPriorityJobs.PushBack(pJobInfo);
		break;
	case eJobPriority::HIGH:
		m_HighPriorityJobs.PushBack(pJobInfo);
		break;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Submits a completed job's results to be handled later.
// Input:	iResultSize - The size in bytes of the resulting data.
//			pJobResults - A pointer to the job result data.
// Output:	none.
//-------------------------------------------------------------------------
void CJobSystem::SubmitJobResults(u64 iResultSize, JobResults_t* pJobResults)
{
	if (iResultSize > 0 && pJobResults)
	{
		// Get a lock on our result mutex and assign the pointer.
		m_pJobResultMutex->Lock();
		m_JobResults.PushBack(*pJobResults);
		m_pJobResultMutex->Unlock();
	}
}

//-------------------------------------------------------------------------
// Purpose: This will force the calling thread to wait for all 
//			registered threads to finish any work they may be doing.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CJobSystem::WaitForWorkerThreadsToFinish() const
{
	bool bWorkDone = false;

	while (!bWorkDone)
	{
		bWorkDone = true;

		for (i32 iThread = 0; iThread < m_pWorkerThreads.Count(); iThread++)
		{
			CWorkerThread& pThread = m_pWorkerThreads[iThread];
			if (pThread.m_CurrentWorkData.pfnEntryPoint != nullptr) // There's some work happening here!
			{
				bWorkDone = false;
				break;
			}

			if (pThread.m_CurrentWorkData.pJobResults.pUserData != nullptr) // The job might be done but the results still need to be processed!
			{
				bWorkDone = false;
				break;
			}
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: This will force the calling thread to wait for all 
//			registered threads to exit.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CJobSystem::WaitForWorkerThreadsToShutdown()
{
	bool bThreadsClosed = false;

	while (!bThreadsClosed)
	{
		bThreadsClosed = true;

		for (i32 iThread = 0; iThread < m_pWorkerThreads.Count(); iThread++)
		{
			CWorkerThread& pThread = m_pWorkerThreads[iThread];
			if (pThread.m_iThreadIdx != kInvalidID_U8)
			{
				// This thread is still going, wait.
				bThreadsClosed = false;
				break;
			}
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Processes the jobs in the given queue and assigns them to worker threads
//			for execution, if possible.
// Input:	pJobQueue - A reference to the queue to be processed.
// Output:	none.
//-------------------------------------------------------------------------
void CJobSystem::ProcessJobQueue(Utl::CDeque<JobDeclaration_t>& pJobQueue)
{
	i32 iJobHeadIdx = pJobQueue.HeadIndex();
	for (i32 iJob = iJobHeadIdx; iJob < (pJobQueue.Count() + iJobHeadIdx); iJob++)
	{
		if (pJobQueue.PeekFront()->pfnEntryPoint)
		{
			CWorkerThread* pThread = &m_pWorkerThreads[0];
			// There's a job here, try to find a worker thread and assign it.
			for (i32 iThread = 0; iThread < m_Config.iNumJobThreads; iThread++)
			{
				pThread = &m_pWorkerThreads[iThread];
				if (pThread->m_iThreadType != pJobQueue[iJob].iJobType || pThread->m_CurrentWorkData.pfnEntryPoint)
				{
					// Either the current thread does not support the current job, or it is in use.
					continue;
				}
				else
				{
					JobDeclaration_t job = pJobQueue.PopFront();

					// This thread can support the job. Set it up.
					pThread->m_pWorkDataMutex->Lock();
					pThread->m_CurrentWorkData = job;
					pThread->m_pWorkDataMutex->Unlock();
					break;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Processed the results of any finished jobs and executes it's callback.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CJobSystem::ProcessJobResults()
{
	for (i32 iJob = 0; iJob < m_JobResults.Count(); iJob++)
	{
		if (m_JobResults[iJob].pfnOnJobCompleted_Success_Callback)
		{
			// Execute the callback.
			JobResults_t* pTypedResults = &m_JobResults[iJob];

			if (pTypedResults->bWasJobSuccessful)
			{
				if (pTypedResults->pfnOnJobCompleted_Success_Callback)
					pTypedResults->pfnOnJobCompleted_Success_Callback(*pTypedResults);
			}
			else
			{
				if (pTypedResults->pfnOnJobCompleted_Failed_Callback)
					pTypedResults->pfnOnJobCompleted_Failed_Callback(*pTypedResults);
			}

			// Once the job is finished. Kill the results and reset the entry.
			// Note the the job is responsible for freeing pUserData.
			m_JobResults[iJob].pfnOnJobCompleted_Success_Callback = nullptr;
			m_JobResults[iJob].pfnOnJobCompleted_Failed_Callback = nullptr;
			m_JobResults[iJob].iUserDataSize = 0;
			m_JobResults[iJob].pUserData = nullptr;
		}
	}
}
