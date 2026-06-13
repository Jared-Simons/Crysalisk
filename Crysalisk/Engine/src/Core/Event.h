//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: The interface for Event Systems.
//=============================================================================
#pragma once

#include "Defines.h"

//-------------------------------------------------------------------------
// Core engine event codes.
//-------------------------------------------------------------------------
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

const i32 kMaxEventsPerCategory = 32;
const i32 kNumEventCategories = static_cast<i32>(eEventCategory::MAX_EVENTS);

//-------------------------------------------------------------------------
// A somewhat generic structure to store event data.
//-------------------------------------------------------------------------
union EventData_t
{
	u64		u64[2];
	i64		i64[2];
	f64		f64[2];

	u32		u32[4];
	i32		i32[4];
	f32		f32[4];

	u16		u16[8];
	i8		i16[8];

	u8		u8[16];
	i8		i8[16];

	char	c[16];
};

//-------------------------------------------------------------------------
// Function pointer signature that all event callbacks must match.
// 
// NOTE: Returning true from this callback means that the event is considered
// 'handled', and will not propagate to other listeners, returning false means
// that the event will continue to pass through to all listeners.
//-------------------------------------------------------------------------
typedef bool (*Event_Callback)(void* pEventListener, EventData_t data);

struct Event
{
	// The callback function to handle the event.
	Event_Callback m_PFNEventCallback;
	// A pointer to the event listener.
	void* pEventListener;
	// An event ID that is maintained internally by the event system.
	i32 m_EventID;
	// The event category that this event is listening to.
	eEventCategory m_EventCategory;
};

//-------------------------------------------------------------------------
// Interface for an event system.
//-------------------------------------------------------------------------
class IEventSystem
{
public:
	static void			AllocateEventSystemState(IEventSystem** ppSystemState);

	// Initialization, shutdown.
	virtual bool		Initialize() = 0;
	virtual void		Shutdown() = 0;

	virtual bool		EventRegister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) = 0;
	virtual bool		EventUnregister(eEventCategory eventCategory, void* listenerInst, Event_Callback PFN_Callback) = 0;
	virtual void		EventFire(eEventCategory eventCategory, void* senderInst, EventData_t data) = 0;
};
