#include "CEvent.h"

#include "Core/Logging.h"

bool CEventSystem::EventSystem_Initialize()
{
	// Loop through and invalidate event entries.
	for (int i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (int j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
		{
			m_RegisteredEvents[i][j].m_EventCategory = eEventCategory::EVENT_TYPE_UNKNOWN;
			m_RegisteredEvents[i][j].m_EventID = -1;
			m_RegisteredEvents[i][j].m_PFNEventCallback = 0;
		}

		m_numRegisteredEvents[i] = 0;
	}

	LOG_INFO("Event System Initialized!");
	return true;
}

void CEventSystem::EventSystem_Shutdown()
{
	LOG_INFO("Event System Shutting Down");

	for (int i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (int j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
		{
			if (m_RegisteredEvents[i][j].m_EventID != -1)
			{
				LOG_ERROR("Event System shutting down with events still registered, clean this up...");
			}
		}
	}
}

bool CEventSystem::EventRegister(eEventCategory eventCategory, Event_Callback PFN_Callback)
{
	if (eventCategory == eEventCategory::EVENT_TYPE_UNKNOWN || !PFN_Callback)
	{
		LOG_ERROR("CEventSystem::EventRegister() requires a valid Event Category and Callback function pointer.");
		return false;
	}

	int intEventCategory = static_cast<int>(eventCategory);
	Event* newEvent = &m_RegisteredEvents[intEventCategory][0];

	// Iterate through the registered events (of the same category) until a free slot is found
	for (m_numRegisteredEvents[intEventCategory]; m_numRegisteredEvents[intEventCategory] < MAX_EVENTS_PER_CATEGORY; m_numRegisteredEvents[intEventCategory]++)
	{
		if (newEvent[m_numRegisteredEvents[intEventCategory]].m_EventID == -1)
		{
			// Found a free slot, populate
			newEvent = &newEvent[m_numRegisteredEvents[intEventCategory]];

			newEvent->m_EventID = m_numRegisteredEvents[intEventCategory] + 1;
			newEvent->m_EventCategory = eventCategory;
			newEvent->m_PFNEventCallback = PFN_Callback;

			m_numRegisteredEvents[intEventCategory]++;
			return true;
		}
	}

	// If we make it here, then there are no more event slots available.
	LOG_ERROR("Cannot register new event, m_RegisteredEvents is full!");
	return false;
}

bool CEventSystem::EventUnregister(eEventCategory eventCategory, Event_Callback PFN_Callback)
{
	if (eventCategory == eEventCategory::EVENT_TYPE_UNKNOWN || !PFN_Callback)
	{
		LOG_ERROR("CEventSystem::EventUnregister() - requires a valid Event Category and Callback function pointer.");
		return false;
	}

	int intEventCategory = static_cast<int>(eventCategory);
	Event* eventToBeFreed = &m_RegisteredEvents[intEventCategory][0];

	for (int i = 0; i < MAX_EVENTS_PER_CATEGORY; i++)
	{
		if (eventToBeFreed[i].m_EventCategory == eventCategory && eventToBeFreed[i].m_PFNEventCallback == PFN_Callback)
		{
			// Found the corresponding event entry, unregister and uninitialize.
			eventToBeFreed[i].m_EventCategory = eEventCategory::EVENT_TYPE_UNKNOWN;
			eventToBeFreed[i].m_PFNEventCallback = nullptr;
			eventToBeFreed[i].m_EventID = -1;

			return true;
		}
		else
			continue;

		LOG_ERROR("EventSystem::EventUnregister() - requested event but can't be found. No action taken.");
		return false;
	}
}

bool CEventSystem::EventFire(eEventCategory eventCategory, void* data)
{
	int intEventCategory = static_cast<int>(eventCategory);
	Event* curEvent = m_RegisteredEvents[intEventCategory];

	for (int i = 0; i < MAX_EVENTS_PER_CATEGORY; i++)
	{
		curEvent[i].m_PFNEventCallback(data);
		break;
	}

	return true;
}

static CEventSystem sEngineEvents;
IEventSystem* gpEngineEvents = &sEngineEvents;

