//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Contains the basic skeleton class for performing unit tests.
//=============================================================================
#pragma once

#include <Defines.h>

#include <Containers/CVector.h>
#include <Core/Clock.h>
#include <Core/Logging.h>

typedef int (*PFN_UnitTestFunc)();

// Use these macros to state if your test passed or failed
#define FAIL 0
#define PASS 1
#define SKIP 2

class UnitTestGroup
{
public:
	UnitTestGroup(const char* debugTestName)
		: m_debugTestName{ debugTestName } {}

	bool RegisterTestFunc(PFN_UnitTestFunc newTestFunc, const char* testDesc, bool bIsSTLTest) 
	{ 
		if (bIsSTLTest)
		{
			m_STLTests.PushBack(newTestFunc);
		}
		else
		{
			m_RegisteredTests.PushBack(newTestFunc);
			m_NumTests++; // NOTE: We only care about our own container's tests, ignore STL tests in this count.
		}
		m_RegisteredTestNames.PushBack(testDesc); 
		return true; 
	}
	void PerformTests()
	{
		m_TestClock.Start();

		for (int i = 0; i < m_RegisteredTests.Count(); i++)
		{
			Clock singleClock;
			singleClock.Start();
			bool passed = false;
			if (m_RegisteredTests[i]() == 1)
			{
				m_TestsPassed++;
				passed = true;
			}
			else
				m_TestsFailed++;

			singleClock.Stop();

			if (passed)
			{
				LOG_DEBUG("[%s]\t\t\t\t\t\t %s\t(Time Elapsed: %.10f)", "PASS", m_RegisteredTestNames[i], singleClock.GetElapsedTime());
			}
			else
			{
				LOG_ERROR("[%s]\t\t\t\t\t\t %s\t(Time Elapsed: %.10f)", "FAIL", m_RegisteredTestNames[i], singleClock.GetElapsedTime());
			}

			// Perform the equivalent STL test.
			singleClock.Start();
			if (m_STLTests[i]() == 1)
			{
				passed = true;
			}
			else
			{
				passed = false;
			}

			if (passed)
			{
				LOG_DEBUG("[%s - STL Equivalent]\t\t\t %s\t(Time Elapsed: %.10f)", "PASS", m_RegisteredTestNames[i], singleClock.GetElapsedTime());
			}
			else
			{
				LOG_ERROR("[%s - STL Equivalent]\t\t\t %s\t(Time Elapsed: %.10f)", "FAIL", m_RegisteredTestNames[i], singleClock.GetElapsedTime());
			}

			singleClock.Stop();
		}

		m_TestClock.Stop();
		PrintTestResults();
	}

	

	void PrintTestResults()
	{
		LOG_INFO("%s Results: %d/%d Passed | %d/%d Failed | %d/%d Skipped (Time Elapsed: %f)", m_debugTestName, m_TestsPassed, m_NumTests, m_TestsFailed, m_NumTests, m_TestsSkipped, m_NumTests, m_TestClock.GetElapsedTime());
	}

private:
	Utl::CVector<PFN_UnitTestFunc> m_RegisteredTests;
	Utl::CVector<PFN_UnitTestFunc> m_STLTests;
	Utl::CVector<const char*> m_RegisteredTestNames;
	Clock m_TestClock;

	const char*	 m_debugTestName = nullptr;
	int m_NumTests = 0;
	int m_TestsPassed = 0;
	int m_TestsFailed = 0;
	int m_TestsSkipped = 0;
};

