//=============================================================================
// Author: Jared Simons
// Date Created: 11/23/2025
// 
// Purpose: The job system.
//=============================================================================
#include "JobSystem.h"

#include <Containers/CVector.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>

struct CWorkerThread
{
	Platform::ISystemThread*	m_pThread{};
	JobDeclaration_t			m_CurrentWorkData{};
};

class CJobSystem : public IJobSystem
{
public:
	virtual bool	Initialize(JobSystemConfig_t& pConfig) override;
	virtual void	Shutdown() override;

	virtual bool	AddJob(JobDeclaration_t& pJobInfo) override;

private:
	JobSystemConfig_t			m_Config{};
	bool						m_bJobSystemActive{ false };

	Utl::CVector<CWorkerThread>	m_pWorkerThreads{};

	friend static int JobThread_Main(void* pParams);
};

// Job System context.
static CJobSystem s_JobSystem;
IJobSystem* g_pJobSystem = &s_JobSystem;

static int JobThread_Main(void* pParams)
{
	LOG_DEBUG("Spawning job thread ID: %d", Platform::GetCurrentThreadID());
	while (s_JobSystem.m_bJobSystemActive)
	{
	}

	LOG_WARN("Killing job thread %d", Platform::GetCurrentThreadID());
	return 0;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CJobSystem::Initialize(JobSystemConfig_t& pConfig)
{
	m_Config = pConfig;
	m_bJobSystemActive = true;

	// Make sure we have at least one additional thread (aside from the main thread) to do work on.
	if (pConfig.iNumJobThreads < 1)
	{
		LOG_FATAL("One core detected, at least 2 cores are required. Job system initialization failed.");
		return false;
	}
	m_pWorkerThreads.Resize(pConfig.iNumJobThreads);

	for (int iThread = 0; iThread < pConfig.iNumJobThreads; iThread++)
	{
		m_pWorkerThreads[iThread].m_pThread = Platform::CreateSystemThread(JobThread_Main, nullptr);
		m_pWorkerThreads[iThread].m_pThread->m_iThreadID = iThread;
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

	// THREADTODO: Should this be released through the platform layer as well?
	m_pWorkerThreads.~CVector();
}

//-------------------------------------------------------------------------
// THREADTODO: Documentation.
//-------------------------------------------------------------------------
bool CJobSystem::AddJob(JobDeclaration_t& pJobInfo)
{
	return true;
}

