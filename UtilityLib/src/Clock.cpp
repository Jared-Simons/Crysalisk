//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: A simple clock utility.
//=============================================================================
#include "Clock.h"

#include <Core/Logging.h>
#include <Platform/Platform.h>

void Clock::Start()
{
	m_StartTime = Platform::GetAbsoluteTime();
	m_IsActive = true;
}

void Clock::Stop()
{
	Update();
	m_IsActive = false;
}

void Clock::Update()
{
	if (m_IsActive)
	{
		f64 currentTime = Platform::GetAbsoluteTime();
		m_ElapsedTime = currentTime - m_StartTime;
	}
}

f64 Clock::GetElapsedTime()
{
	if (!m_IsActive)
	{
		return m_ElapsedTime;
	}
	else
	{
		Update();
		return m_ElapsedTime;
	}
}

