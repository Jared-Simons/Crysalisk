//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: The Interface for Event Systems.
//=============================================================================
#pragma once

#include <Memory/LinearAllocator.h>

// Forward Declarations
class LinearAllocator;

const int MAX_EVENTS_PER_CATEGORY = 32;
#define NUM_EVENT_CATEGORIES static_cast<int>(eEventCategory::MAX_EVENTS)

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


union EventData_t
{
	unsigned long long	u64[2];
	signed	 long long	i64[2];
	double				f64[2];

	unsigned long		u32[4];
	signed	 long		i32[4];
	float				f32[4];

	unsigned short		u16[8];
	signed   short		i16[8];

	unsigned char		u8[16];
	signed   char		i8[16];

	char				c[16];
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

