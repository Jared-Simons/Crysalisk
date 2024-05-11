#pragma once

class Clock
{
public:
	Clock()
		: m_StartTime{ 0 }
		, m_ElapsedTime{ 0 }
		, m_IsActive{ false } {}

	void Start();
	void Stop();
	void Update();

	double GetElapsedTime();
	double GetStartTime() { return m_StartTime; }

	bool IsClockActive() { return m_IsActive; }

private:
	double	m_StartTime;
	double	m_ElapsedTime;
	bool	m_IsActive;
};

