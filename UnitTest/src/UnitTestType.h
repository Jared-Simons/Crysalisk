//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Contains the basic skeleton class for performing unit tests.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include <Clock.h>
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

	bool RegisterTestFunc(PFN_UnitTestFunc newTestFunc, const char* testDesc) { m_RegisteredTests.PushBack(newTestFunc); m_RegisteredTestNames.PushBack(testDesc); return true; }
	void PerformTests()
	{
		m_TestClock.Start();

		for (int i = 0; i < m_RegisteredTests.GetCount(); i++)
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

			LOG_VERBOSE("%s\t\t\t\t [%s]", m_RegisteredTestNames[i], passed ? "PASS" : "FAIL");
		}

		m_TestClock.Stop();
		PrintTestResults();
	}

	

	void PrintTestResults()
	{
		LOG_INFO("%s Results: %d/%d Passed | %d/%d Failed | %d/%d Skipped (Time Elapsed: %f)", m_debugTestName, m_TestsPassed, m_NumTests, m_TestsFailed, m_NumTests, m_TestsSkipped, m_NumTests, m_TestClock.GetElapsedTime());
	}

private:
	CVector<PFN_UnitTestFunc> m_RegisteredTests;
	CVector<const char*> m_RegisteredTestNames;
	Clock m_TestClock;

	const char*	 m_debugTestName = nullptr;
	unsigned int m_NumTests = 0;
	unsigned int m_TestsPassed = 0;
	unsigned int m_TestsFailed = 0;
	unsigned int m_TestsSkipped = 0;
};

