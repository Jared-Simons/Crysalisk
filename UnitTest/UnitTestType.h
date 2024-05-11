//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Contains the basic skeleton class for performing unit tests.
//=============================================================================
#pragma once

#include <CVector.h>

typedef int (*PFN_UnitTestFunc)();

class UnitTestGroup
{
public:
	bool RegisterTestFunc(PFN_UnitTestFunc newTestFunc) { m_RegisteredTests.PushBack(newTestFunc); }
	void PerformTests()
	{
		for (int i = 0; i < m_RegisteredTests.GetSize(); i++)
		{
			if (m_RegisteredTests[i]())
				m_TestsPassed++;
			else
				m_TestsFailed++;
		}
	}

	unsigned int GetNumOfTests() const { return m_NumTests; }
	unsigned int GetNumOfTestsPassed() const { return m_TestsPassed; }
	unsigned int GetNumOfTestsFailed() const { return m_TestsFailed; }
	unsigned int GetNumOfTestsSkipped() const { return m_TestsSkipped; }

private:
	CVector<PFN_UnitTestFunc> m_RegisteredTests;

	unsigned int m_NumTests = 0;
	unsigned int m_TestsPassed = 0;
	unsigned int m_TestsFailed = 0;
	unsigned int m_TestsSkipped = 0;
};

