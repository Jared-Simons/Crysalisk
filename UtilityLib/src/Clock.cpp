#include "Clock.h"

#include <Core/Logging.h>
#include <Platform/Platform.h>

void Clock::Start()
{
	m_StartTime = Platform_GetAbsoluteTime();
	m_IsActive = true;
}

void Clock::Stop()
{
	m_IsActive = false;
}

void Clock::Update()
{
	if (m_IsActive)
	{
		double currentTime = Platform_GetAbsoluteTime();
		m_ElapsedTime = currentTime - m_StartTime;
	}
}

double Clock::GetElapsedTime()
{
	if (!m_IsActive)
	{
		LOG_ERROR("Clock - GetElaspedTime() - Clock not initialized!");
		return -1;
	}
	else
	{
		Update();
		return m_ElapsedTime;
	}
}

