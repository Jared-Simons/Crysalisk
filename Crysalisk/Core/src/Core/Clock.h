//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: A simple clock utility.
//=============================================================================
#pragma once

#include <Defines.h>

//-------------------------------------------------------------------------
// A simple, humble clock.
//-------------------------------------------------------------------------
class CORE_API Clock
{
public:
	Clock()
		: m_StartTime{ 0 }
		, m_ElapsedTime{ 0 }
		, m_IsActive{ false } {}

	void Start();
	void Stop();
	void Update();

	f64 GetElapsedTime();
	f64 GetStartTime() { return m_StartTime; }

	bool IsClockActive() { return m_IsActive; }

private:
	f64	m_StartTime;
	f64	m_ElapsedTime;
	bool	m_IsActive;
};

