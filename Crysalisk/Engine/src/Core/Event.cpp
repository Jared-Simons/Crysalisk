#include "Event.h"

#include <Core/Logging.h>

//-------------------------------------------------------------------------
// Implementation of the engine's event system.
//-------------------------------------------------------------------------
class EventSystem : public IEventSystem
{
public:
	virtual bool		Initialize() override;
	virtual void		Shutdown() override;

	virtual bool		EventRegister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) override;
	virtual bool		EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) override;
	virtual void		EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data) override;

private:
	Event m_RegisteredEvents[kNumEventCategories][kMaxEventsPerCategory];
	int m_numRegisteredEvents[kNumEventCategories];
};

// Event system context.
EventSystem s_EventSystem;
IEventSystem* g_pEngineEvents_Internal = &s_EventSystem;

//-------------------------------------------------------------------------
// Purpose - Initializes the event system.
// Input - none
// Output - bool: Returns true on success; otherwise false.
//-------------------------------------------------------------------------
bool EventSystem::Initialize()
{
	// Loop through and invalidate event entries.
	for (int i = 0; i < kNumEventCategories; i++)
	{
		for (int j = 0; j < kMaxEventsPerCategory; j++)
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

//-------------------------------------------------------------------------
// Purpose - Shuts down the event system.
//-------------------------------------------------------------------------
void EventSystem::Shutdown()
{
}

//-------------------------------------------------------------------------
// Purpose - Registers a callback to the event system.
// Input - eventCategory: The event category to listen for. (I.E., Window-Resized, Mouse-Moved).
//		   listenerInst: An opaque pointer to some listener to provide additional context to the callback (Optional).
//		   PFN_Callback: A function pointer to the event-handler to be called on event firing.
// Output - bool: Returns true on event successfully registered, otherwise false.
//-------------------------------------------------------------------------
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
	for (int i = 0; i < kMaxEventsPerCategory;i++)
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

//-------------------------------------------------------------------------
// Purpose - Unregisters a callback from the event system.
// Input - eventCategory: The event category to stop listening for.
//		   listenerInst: An opaque pointer to some listener to provide additional context to the callback (Optional).
//		   PFN_Callback: A function pointer to the event-handler to be unregistered.
// Output - bool: Returns true on event successfully unregistered, otherwise false.
//-------------------------------------------------------------------------
bool EventSystem::EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback)
{
	if (eventCategory == eEventCategory::UNKNOWN || !PFN_Callback)
	{
		LOG_ERROR("CEventSystem::EventUnregister() - requires a valid Event Category and Callback function pointer.");
		return false;
	}

	int intEventCategory = static_cast<int>(eventCategory);
	Event* eventToBeFreed = &m_RegisteredEvents[intEventCategory][0];

	for (int i = 0; i < kMaxEventsPerCategory; i++)
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

//-------------------------------------------------------------------------
// Purpose - Fires an event out to all registered listeners.
// Input - eventCategory: The event category to fire.
//		   listenerInst: An opaque pointer to the sender instance, to provide additional context in the callback. Optional.
//		   EventData: A structure of generic data that can be used in the callback to provide context (e.g. the new window dimensions in a window resize event).
// Output - void
//-------------------------------------------------------------------------
void EventSystem::EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data)
{
	int intEventCategory = static_cast<int>(eventCategory);
	Event* currEvent = m_RegisteredEvents[intEventCategory];

	for (int i = 0; i < kMaxEventsPerCategory; i++)
	{
		if (currEvent[i].m_EventID != -1)
		{
			currEvent[i].m_PFNEventCallback(data);
		}
	}
}

