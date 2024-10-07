//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: The interface for Event Systems.
//=============================================================================
#pragma once

#include "Defines.h"
#include <Memory/LinearAllocator.h>

// Forward Declarations
class LinearAllocator;

enum class eEventCategory
{
	UNKNOWN = -1,
	WINDOW_RESIZE,
	WINDOW_START_RESIZE,
	WINDOW_END_RESIZE,
	APPLICATION_QUIT,
	INPUT_KEY_PRESSED,
	INPUT_KEY_RELEASED,
	INPUT_MOUSE_BPRESSED,
	INPUT_MOUSE_BRELEASED,
	INPUT_MOUSE_MOVED,

	MAX_EVENTS
};

const int MAX_EVENTS_PER_CATEGORY = 32;
const int NUM_EVENT_CATEGORIES = static_cast<int>(eEventCategory::MAX_EVENTS);

union EventData_t
{
	u64		u64[2];
	i64		i64[2];
	f64		f64[2];

	u32		u32[4];
	int		i32[4];
	f32		f32[4];

	u16		u16[8];
	i8		i16[8];

	u8		u8[16];
	i8		i8[16];

	char	c[16];
};

typedef bool (*Event_Callback)(EventData_t data);

struct Event
{
	Event_Callback m_PFNEventCallback;
	int m_EventID;
	eEventCategory m_EventCategory;
};

class EventSystem
{
public:
	static bool		Initialize();
	static void		Shutdown();

	static bool		EventRegister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback);
	static bool		EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback);
	static bool		EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data);

private:
	static Event m_RegisteredEvents[NUM_EVENT_CATEGORIES][MAX_EVENTS_PER_CATEGORY];
	static int m_numRegisteredEvents[NUM_EVENT_CATEGORIES];
};

