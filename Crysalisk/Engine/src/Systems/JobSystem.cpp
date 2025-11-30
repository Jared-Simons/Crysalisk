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
	JobDeclaration_t			m_CurrentWorkData{};
};

class CJobSystem : public IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) override;
	virtual void	Shutdown() override;
	virtual void	Update() override;

	virtual bool	SubmitJob(JobDeclaration_t& pJobInfo) override;

private:
	JobSystemConfig_t				m_Config{};
	bool							m_bJobSystemActive{ false };

	Utl::CVector<CWorkerThread>		m_pWorkerThreads{};

	Utl::CDeque<JobDeclaration_t>	m_LowPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_NormalPriorityJobs{};
	Utl::CDeque<JobDeclaration_t>	m_HighPriorityJobs{};

	friend static int JobThread_Main(void* pParams);
};

// Job System context.
static CJobSystem s_JobSystem;
IJobSystem* g_pJobSystem = &s_JobSystem;

static int JobThread_Main(void* pParams)
{
	size_t iThreadIdx = *(size_t*)pParams;
	int iThreadID = Platform::GetCurrentThreadID();
	LOG_DEBUG("Spawning job thread ID: %d", iThreadIdx);
	while (s_JobSystem.m_bJobSystemActive)
	{
		if (s_JobSystem.m_pWorkerThreads.Capacity() && s_JobSystem.m_pWorkerThreads[iThreadIdx].m_CurrentWorkData.pfnEntryPoint)
		{
			LOG_WARN("JOB TO BE PROCESSED: %d", iThreadIdx);
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
	m_NormalPriorityJobs.Resize(50);
	m_HighPriorityJobs.Resize(50);

	// Make sure we have at least one additional thread (aside from the main thread) to do work on.
	if (pConfig.iNumJobThreads < 1)
	{
		LOG_FATAL("One core detected, at least 2 cores are required. Job system initialization failed.");
		return false;
	}
	m_pWorkerThreads.Resize(pConfig.iNumJobThreads);
	m_pWorkerThreads.SetCount(pConfig.iNumJobThreads);

	for (size_t iThread = 0; iThread < pConfig.iNumJobThreads; iThread++)
	{
		// THREADTODO: Do we need m_pThread->m_iThreadID?
		//m_pWorkerThreads[iThread].m_pThread->m_iThreadID = iThread;
		m_pWorkerThreads[iThread].m_iThreadIdx = iThread;
		m_pWorkerThreads[iThread].m_pThread = Platform::CreateSystemThread(JobThread_Main, &m_pWorkerThreads[iThread].m_iThreadIdx);
		m_pWorkerThreads[iThread].m_iThreadType = pConfig.iWorkerThreadTypes[iThread];
	}
	
	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CJobSystem::Shutdown()
{
	// This will kill all job threads.
	m_bJobSystemActive = false;

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

	// First, process high priority jobs.
	for (int iJob = 0; iJob < m_HighPriorityJobs.Count(); iJob++)
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
					pThread->m_CurrentWorkData = job;
				}
			}
		}
	}

	// Next, the normal priority jobs.
	for (int iJob = 0; iJob < m_NormalPriorityJobs.Count(); iJob++)
	{
		if (m_NormalPriorityJobs.PeekFront()->pfnEntryPoint)
		{
			JobDeclaration_t job = m_NormalPriorityJobs.PopFront();
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
					pThread->m_CurrentWorkData = job;
					LOG_DEBUG("Placing job");
					break;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
// THREADTODO: Documentation.
//-------------------------------------------------------------------------
bool CJobSystem::SubmitJob(JobDeclaration_t& pJobInfo)
{
	// THREADTODO: What to do if there's no more space in the job queues?
	if (!pJobInfo.pfnEntryPoint)
	{
		LOG_ERROR("CJobSystem::SubmitJob requires pfnEntryPoint to not be NULL. Nothing was done.");
		return false;
	}

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

