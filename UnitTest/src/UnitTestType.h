//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Contains the basic skeleton class for performing unit tests.
//=============================================================================
#pragma once

#include <CVector.h>
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

	bool RegisterTestFunc(PFN_UnitTestFunc newTestFunc) { m_RegisteredTests.PushBack(newTestFunc); return true; }
	void PerformTests()
	{
		m_TestClock.Start();

		for (int i = 0; i < m_RegisteredTests.GetSize(); i++)
		{
			if (m_RegisteredTests[i]() == 1)
				m_TestsPassed++;
			else
				m_TestsFailed++;
		}

		m_TestClock.Stop();
	}

	void PrintTestResults()
	{
		LOG_DEBUG("%s Results: %d/%d Passed | %d/%d Failed | %d/%d Skipped (Time Elapsed: %f)", m_debugTestName, m_TestsPassed, m_NumTests, m_TestsFailed, m_NumTests, m_TestsSkipped, m_NumTests, m_TestClock.GetElapsedTime());
	}

private:
	CVector<PFN_UnitTestFunc> m_RegisteredTests;
	Clock m_TestClock;

	const char*	 m_debugTestName = nullptr;
	unsigned int m_NumTests = 0;
	unsigned int m_TestsPassed = 0;
	unsigned int m_TestsFailed = 0;
	unsigned int m_TestsSkipped = 0;
};

