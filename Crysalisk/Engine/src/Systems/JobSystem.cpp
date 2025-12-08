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

struct CWorkerThread
{
	uint8_t						m_iThreadIdx{ 0 };
	Platform::ISystemThread*	m_pThread{};
	uint32_t					m_iThreadType{ WORKER_THREAD_TYPE_GENERAL };
	Platform::ISystemMutex*		m_pWorkDataMutex{ nullptr };
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

class CJobSystem : public IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) override;
	virtual void	Shutdown() override;
	virtual void	Update() override;

	virtual bool	SubmitJob(JobDeclaration_t& pJobInfo) override;
	virtual void	SubmitJobResults(size_t iResultSize, void* pResults) override;

private:
	JobSystemConfig_t				m_Config{};
	bool							m_bJobSystemActive{ false };

	Utl::CVector<CWorkerThread>		m_pWorkerThreads{};

	Utl::CDeque<JobDeclaration_t>	m_LowPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_NormalPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_HighPriorityJobs{};

	Platform::ISystemMutex*			m_pJobResultMutex{ nullptr };
	Utl::CVector<JobResults_t>		m_JobResults{};	// THREADTODO: void* or uintptr_t?

	friend static int JobThread_Main(void* pParams);
};

// Job System context.
static CJobSystem s_JobSystem;
IJobSystem* g_pJobSystem = &s_JobSystem;

// THREADTODO: Clean this up!
static int JobThread_Main(void* pParams)
{
	uint8_t iThreadIdx = *(uint8_t*)pParams;
	int iThreadID = Platform::GetCurrentThreadID();
	LOG_DEBUG("Spawning job thread ID: %d", iThreadIdx);
	CWorkerThread& pThisThread = s_JobSystem.m_pWorkerThreads[iThreadIdx];

	while (s_JobSystem.m_bJobSystemActive)
	{
		if (s_JobSystem.m_pWorkerThreads.Capacity() && pThisThread.m_CurrentWorkData.pfnEntryPoint)
		{
			// Make sure we get a lock on our resources before executing the job.
			pThisThread.m_pWorkDataMutex->Lock();
			(pThisThread.m_CurrentWorkData.pfnEntryPoint)(pThisThread.m_CurrentWorkData.pParams, &pThisThread.m_CurrentWorkData.pJobResults.pUserData);

			// THREADTODO: Check to make sure the result pointers are not null.
			s_JobSystem.SubmitJobResults(pThisThread.m_CurrentWorkData.pJobResults.iUserDataSize, &pThisThread.m_CurrentWorkData.pJobResults);

			// Clear the job data, so that we don't run it again.
			pThisThread.m_CurrentWorkData.ResetEntry();
			pThisThread.m_pWorkDataMutex->Unlock();
		}
	}

	LOG_WARN("Killing job thread %d", iThreadID);
	return 0;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CJobSystem::Initialize(JobSystemConfig_t& pConfig)
{
	m_Config = pConfig;
	m_bJobSystemActive = true;

	// Reasonable defaults.
	m_LowPriorityJobs.Resize(50);
	m_NormalPriorityJobs.Resize(80);	// THREADTODO: Real defaults.
	m_HighPriorityJobs.Resize(50);

	// Make sure we have at least one additional thread (aside from the main thread) to do work on.
	if (pConfig.iNumJobThreads < 1)
	{
		LOG_FATAL("One core detected, at least 2 cores are required. Job system initialization failed.");
		return false;
	}
	m_pWorkerThreads.Resize(pConfig.iNumJobThreads);
	m_pWorkerThreads.SetCount(pConfig.iNumJobThreads);

	for (uint8_t iThread = 0; iThread < pConfig.iNumJobThreads; iThread++)
	{
		m_pWorkerThreads[iThread].m_iThreadIdx = iThread;
		m_pWorkerThreads[iThread].m_pThread = Platform::CreateSystemThread(JobThread_Main, &m_pWorkerThreads[iThread].m_iThreadIdx);
		m_pWorkerThreads[iThread].m_pWorkDataMutex = Platform::CreateSystemMutex();
		m_pWorkerThreads[iThread].m_iThreadType = pConfig.iWorkerThreadTypes[iThread];
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
	m_bJobSystemActive = false;

	m_JobResults.Clear();
	Platform::DestroySystemMutex(m_pJobResultMutex);
	m_pJobResultMutex = nullptr;

	// THREADTODO: Make this threadsafe.
	m_pWorkerThreads.Clear();
	m_LowPriorityJobs.Clear();
	m_NormalPriorityJobs.Clear();
	m_HighPriorityJobs.Clear();
}

//-------------------------------------------------------------------------
// Processes the job queues and kicks off any work to be done.
//-------------------------------------------------------------------------
void CJobSystem::Update()
{
	// Process job queues and assign to worker threads.

	// THREADTODO: D.R.Y.
	// First, process high priority jobs.
	int iHighJobHead = m_HighPriorityJobs.HeadIndex();
	for (int iJob = iHighJobHead; iJob < (m_HighPriorityJobs.Count() + iHighJobHead); iJob++)
	{
		if (m_HighPriorityJobs.PeekFront()->pfnEntryPoint)
		{
			JobDeclaration_t job = m_HighPriorityJobs.PopFront();
			CWorkerThread* pThread = &m_pWorkerThreads[0];
			// There's a job here, try to find a worker thread and assign it.
			for (int iThread = 0; iThread < m_Config.iNumJobThreads; iThread++)
			{
				pThread = &m_pWorkerThreads[iThread];
				if (pThread->m_iThreadType != job.iJobType || pThread->m_CurrentWorkData.pfnEntryPoint)
				{
					// Either the current thread does not support the current job, or it is in use.
					continue;
				}
				else
				{
					// This thread can support the job. Set it up.
					pThread->m_pWorkDataMutex->Lock();
					pThread->m_CurrentWorkData = job;
					pThread->m_pWorkDataMutex->Unlock();
				}
			}
		}
	}

	// Next, the normal priority jobs.
	int iNormalJobHead = m_NormalPriorityJobs.HeadIndex();
	for (int iJob = iNormalJobHead; iJob < (m_NormalPriorityJobs.Count() + iNormalJobHead); iJob++)
	{
		if (m_NormalPriorityJobs.PeekFront()->pfnEntryPoint)
		{
			CWorkerThread* pThread = &m_pWorkerThreads[0];
			// There's a job here, try to find a worker thread and assign it.
			for (int iThread = 0; iThread < m_Config.iNumJobThreads; iThread++)
			{
				pThread = &m_pWorkerThreads[iThread];
				if (pThread->m_iThreadType != m_NormalPriorityJobs[iJob].iJobType || pThread->m_CurrentWorkData.pfnEntryPoint)
				{
					// Either the current thread does not support the current job, or it is in use.
					continue;
				}
				else
				{
					JobDeclaration_t job = m_NormalPriorityJobs.PopFront();

					// This thread can support the job. Set it up.
					pThread->m_pWorkDataMutex->Lock();
					pThread->m_CurrentWorkData = job;
					pThread->m_pWorkDataMutex->Unlock();

					LOG_WARN("Jobs remaining: %d", m_NormalPriorityJobs.Count());
					break;
				}
			}
		}
	}

	// Finally, low priority jobs.
	int iLowJobHead = m_LowPriorityJobs.HeadIndex();
	for (int iJob = iLowJobHead; iJob < (m_LowPriorityJobs.Count() + iLowJobHead); iJob++)
	{
		if (m_LowPriorityJobs.PeekFront()->pfnEntryPoint)
		{
			JobDeclaration_t job = m_LowPriorityJobs.PopFront();
			CWorkerThread* pThread = &m_pWorkerThreads[0];
			// There's a job here, try to find a worker thread and assign it.
			for (int iThread = 0; iThread < m_Config.iNumJobThreads; iThread++)
			{
				pThread = &m_pWorkerThreads[iThread];
				if (pThread->m_iThreadType != job.iJobType || pThread->m_CurrentWorkData.pfnEntryPoint)
				{
					// Either the current thread does not support the current job, or it is in use.
					continue;
				}
				else
				{
					// This thread can support the job. Set it up.
					pThread->m_pWorkDataMutex->Lock();
					pThread->m_CurrentWorkData = job;
					pThread->m_pWorkDataMutex->Unlock();
					break;
				}
			}
		}
	}

	// After kicking off jobs, check for finished jobs.
	m_pJobResultMutex->Lock();
	for (int iJob = 0; iJob < m_JobResults.Count(); iJob++)
	{
		if (m_JobResults[iJob].pfnOnJobCompleted_Success_Callback)
		{
			// Execute the callback.
			// THREADTODO: Success/Fail options?
			JobResults_t* pTypedResults = &m_JobResults[iJob];
			pTypedResults->pfnOnJobCompleted_Success_Callback(*pTypedResults);

			// Once the job is finished. Kill the results and reset the entry.
			// THREADOTODO: Figure out how to free this.
			//g_pMemoryManager->Free(m_JobResults[iJob]);
			m_JobResults[iJob].pfnOnJobCompleted_Success_Callback = nullptr;
			m_JobResults[iJob].iUserDataSize = 0;
			m_JobResults[iJob].pUserData = nullptr;
		}
	}
	m_pJobResultMutex->Unlock();
}

//-------------------------------------------------------------------------
// THREADTODO: Documentation.
//-------------------------------------------------------------------------
bool CJobSystem::SubmitJob(JobDeclaration_t& pJobInfo)
{
	CAssert(m_NormalPriorityJobs.Count() + 1 < m_NormalPriorityJobs.Capacity());
	// THREADTODO: What to do if there's no more space in the job queues?
	if (!pJobInfo.pfnEntryPoint)
	{
		LOG_ERROR("CJobSystem::SubmitJob requires pfnEntryPoint to not be NULL. Nothing was done.");
		return false;
	}

	// Take a copy of the job info.
	JobDeclaration_t jobCopy = pJobInfo;
	// THREADTODO: Free this!
	if (pJobInfo.iParamSize > 0 && pJobInfo.pParams)
	{
		jobCopy.pParams = g_pMemoryManager->Allocate(pJobInfo.iParamSize);
		Platform::MemoryCopy(jobCopy.pParams, pJobInfo.pParams, pJobInfo.iParamSize);
	}

	switch (pJobInfo.ePriority)
	{
		int newJob;
	case eJobPriority::LOW:
		newJob = m_LowPriorityJobs.PushBack(jobCopy);
		break;
	case eJobPriority::NORMAL:
		newJob = m_NormalPriorityJobs.PushBack(jobCopy);
		break;
	case eJobPriority::HIGH:
		newJob = m_HighPriorityJobs.PushBack(jobCopy);
		break;
	}

	return true;
}

//-------------------------------------------------------------------------
// THREADTODO: Documentation.
//-------------------------------------------------------------------------
void CJobSystem::SubmitJobResults(size_t iResultSize, void* pJobResults)
{
	if (iResultSize > 0 && pJobResults)
	{
		LOG_DEBUG("Submitting job results");

		// Allocate new memory for the results.
		// THREADTODO: We should probably be pooling these.
		//void* pResultCopy = g_pMemoryManager->Allocate(iResultSize);
		JobResults_t pResultCopy = *(JobResults_t*)pJobResults;

		// Copy the results over.
		//Platform::MemoryCopy(pResultCopy, pJobResults, iResultSize);

		// Get a lock on our result mutex and assign the pointer.
		m_pJobResultMutex->Lock();
		m_JobResults.PushBack(&pResultCopy);
		m_pJobResultMutex->Unlock();
	}
}

