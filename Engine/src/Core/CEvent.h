//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: Implementation for the Engine-Level event system.
//=============================================================================
#pragma once

#include "Systems/IEvent.h"

const int MAX_EVENTS_PER_CATEGORY = 32;
#define NUM_EVENT_CATEGORIES static_cast<int>(eEventCategory::EVENT_TYPE_MAX_EVENTS)

class Event
{
	friend class CEventSystem;

	Event_Callback m_PFNEventCallback;
	int m_EventID;
	eEventCategory m_EventCategory;
};

class CEventSystem : public IEventSystem
{
public:
	CEventSystem() : m_numRegisteredEvents{ 0 } {}

	bool	EventSystem_Initialize() override;
	void	EventSystem_Shutdown() override;

	bool	EventRegister(eEventCategory eventCategory, Event_Callback PFN_Callback) override;
	bool	EventUnregister(eEventCategory eventCategory, Event_Callback PFN_Callback) override;
	bool	EventFire(eEventCategory eventCategory, void* eventData) override;

private:
	Event m_RegisteredEvents[NUM_EVENT_CATEGORIES][MAX_EVENTS_PER_CATEGORY];
	int	m_numRegisteredEvents[NUM_EVENT_CATEGORIES];
};

