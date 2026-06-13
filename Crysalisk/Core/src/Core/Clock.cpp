//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: A simple clock utility.
//=============================================================================
#include "Clock.h"

#include <Core/Logging.h>
#include <Platform/Platform.h>

//-------------------------------------------------------------------------
// Purpose: Starts (and resets) the clock.
//-------------------------------------------------------------------------
void Clock::Start()
{
	m_StartTime = Platform::GetAbsoluteTime();
	m_IsActive = true;
}

//-------------------------------------------------------------------------
// Purpose: Stops (and updates) the clock.
//-------------------------------------------------------------------------
void Clock::Stop()
{
	Update();
	m_IsActive = false;
}

//-------------------------------------------------------------------------
// Purpose: Updates the clock.
// This should be called before getting the elapsed time from the clock.
//-------------------------------------------------------------------------
void Clock::Update()
{
	if (m_IsActive)
	{
		f64 currentTime = Platform::GetAbsoluteTime();
		m_ElapsedTime = currentTime - m_StartTime;
	}
}

//-------------------------------------------------------------------------
// Purpose: Returns the elapsed time.
//-------------------------------------------------------------------------
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

