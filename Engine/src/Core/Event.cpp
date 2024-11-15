#include "Event.h"

#include "Core/Logging.h"

class EventSystem : public IEventSystem
{
public:
	void* operator new(u64 size, LinearAllocator* pAllocator)
	{
		void* pBlock = pAllocator->Allocate(size);
		if (pBlock)
		{
			return pBlock;
		}
		else
		{
			LOG_FATAL("Systems allocator failed to allocate memory for the event system!");
			return nullptr;
		}
	}

	// NOTE: This does nothing and just exists to clear a compiler warning, 
	// the memory was allocated and will be freed when the linear allocator gets cleared.
	// If the creation of this system fails, the engine will shutdown regardless.
	void	operator delete(void* pMemory, LinearAllocator* pAllocator) {}

	bool		Initialize() override;
	void		Shutdown() override;

	bool		EventRegister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) override;
	bool		EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) override;
	bool		EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data) override;

private:
	Event m_RegisteredEvents[NUM_EVENT_CATEGORIES][MAX_EVENTS_PER_CATEGORY];
	i32 m_numRegisteredEvents[NUM_EVENT_CATEGORIES];
};

class EventSystem* s_EventSystem = nullptr;
class IEventSystem* gpEvents = s_EventSystem;

IEventSystem* IEventSystem::Create(LinearAllocator* pAllocator)
{
	if (pAllocator)
	{
		if (s_EventSystem)
		{
			LOG_ERROR("IEventSystem::Create -> An Event System is already initialized! Nothing was done");
			return nullptr;
		}

		EventSystem* eventSystem = new (pAllocator) EventSystem();

		if (!eventSystem->Initialize())
		{
			return nullptr;
		}

		s_EventSystem = eventSystem;
		return s_EventSystem;
	}

	return nullptr;
}

bool EventSystem::Initialize()
{
	// Loop through and invalidate event entries.
	for (i32 i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (i32 j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
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

	for (i32 i = 0; i < NUM_EVENT_CATEGORIES; i++)
	{
		for (i32 j = 0; j < MAX_EVENTS_PER_CATEGORY; j++)
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

	i32 intEventCategory = static_cast<i32>(eventCategory);
	Event* newEvent = &m_RegisteredEvents[intEventCategory][0];

	// Iterate through the registered events (of the same category) until a free slot is found
	for (i32 i = 0; i < MAX_EVENTS_PER_CATEGORY;i++)
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

	i32 intEventCategory = static_cast<i32>(eventCategory);
	Event* eventToBeFreed = &m_RegisteredEvents[intEventCategory][0];

	for (i32 i = 0; i < MAX_EVENTS_PER_CATEGORY; i++)
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
	i32 intEventCategory = static_cast<i32>(eventCategory);
	Event* currEvent = m_RegisteredEvents[intEventCategory];

	for (i32 i = 0; i < MAX_EVENTS_PER_CATEGORY; i++)
	{
		// TODO: Implement some kind of sorting system, so that we don't have to loop through the ENTIRE array to fire messages.
		// Perhaps we should periodically move events back as far as possible in the array so that all valid events are contiguous,
		// therefore, once an invalid eventid is encountered (-1), we can break out of the loop.
		if (currEvent[i].m_EventID != -1)
		{
			currEvent[i].m_PFNEventCallback(data);
		}
	}

	return true;
}

