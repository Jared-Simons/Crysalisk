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

class IEventSystem
{
public:
	static bool		EventSystem_Create(IEventSystem* statePtr, LinearAllocator* pAllocator);
	static void		EventSystem_Shutdown(IEventSystem* statePtr, bool bOwnsMemeory = false);

	virtual bool	Initialize() = 0;
	virtual void	Shutdown() = 0;

	virtual bool	EventRegister(eEventCategory eventCategory, Event_Callback PFN_Callback) = 0;
	virtual bool	EventUnregister(eEventCategory eventCategory, Event_Callback PFN_Callback) = 0;
	virtual bool	EventFire(eEventCategory eventCategory, EventData_t data) = 0;
};

extern IEventSystem* gpEngineEvents;

