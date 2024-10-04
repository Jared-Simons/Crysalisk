#include "Event.h"

#include "Core/Logging.h"

#include <string>

// Event System Static variables

// TODO: Make this a pointer or something so we can control lifetime.
Event EventSystem::m_RegisteredEvents[NUM_EVENT_CATEGORIES][MAX_EVENTS_PER_CATEGORY];
int	EventSystem::m_numRegisteredEvents[NUM_EVENT_CATEGORIES];

bool EventSystem::Initialize()
{
	// Loop through and invalidate event entries.
	for (int i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (int j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
		{
			m_RegisteredEvents[i][j].m_EventCategory = eEventCategory::UNKNOWN;
			m_RegisteredEvents[i][j].m_EventID = -1;
			m_RegisteredEvents[i][j].m_PFNEventCallback = 0;
		}

		m_numRegisteredEvents[i] = 0;
	}

	LOG_INFO("Event System Initialized!");
	return true;
}

void EventSystem::Shutdown()
{
	LOG_INFO("Event System Shutting Down");

	for (int i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (int j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
		{
			if (m_RegisteredEvents[i][j].m_EventID != -1)
			{
				LOG_WARN("Event System shutting down with events still registered, clean this up...");
			}
		}
	}
}

bool EventSystem::EventRegister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback)
{
	if (eventCategory == eEventCategory::UNKNOWN || !PFN_Callback)
	{
		LOG_ERROR("CEventSystem::EventRegister() requires a valid Event Category and Callback function pointer.");
		return false;
	}

	int intEventCategory = static_cast<int>(eventCategory);
	Event* newEvent = &m_RegisteredEvents[intEventCategory][0];

	// Iterate through the registered events (of the same category) until a free slot is found
	for (int i = 0; i < MAX_EVENTS_PER_CATEGORY;i++)
	{
		// If the event has the same type and callback as another registered event, boot out and log a message
		// We do not want multiple copies of the same event
		if (newEvent[i].m_PFNEventCallback == PFN_Callback)
		{
			LOG_ERROR("CEventSystem::EventRegister() - Attempting to register a duplicate event. No action was taken.");
			return false;
		}

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

bool EventSystem::EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback)
{
	if (eventCategory == eEventCategory::UNKNOWN || !PFN_Callback)
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
			eventToBeFreed[i].m_EventCategory = eEventCategory::UNKNOWN;
			eventToBeFreed[i].m_PFNEventCallback = nullptr;
			eventToBeFreed[i].m_EventID = -1;

			m_numRegisteredEvents[intEventCategory]--;
			return true;
		}
		else
			continue;
	}

	LOG_ERROR("CEventSystem::EventUnregister() - requested event but can't be found. No action taken.");
	return false;
}

bool EventSystem::EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data)
{
	int intEventCategory = static_cast<int>(eventCategory);
	Event* curEvent = m_RegisteredEvents[intEventCategory];

	for (int i = 0; i < MAX_EVENTS_PER_CATEGORY; i++)
	{
		// TODO: Implement some kind of sorting system, so that we don't have to loop through the ENTIRE array to fire messages.
		// Perhaps we should periodically move events back as far as possible in the array so that all valid events are contiguous,
		// therefore, once an invalid eventid is encountered (-1), we can break out of the loop.
		if (curEvent[i].m_EventID != -1)
		{
			curEvent[i].m_PFNEventCallback(data);
		}
	}

	return true;
}

