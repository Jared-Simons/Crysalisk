//=============================================================================
// Author: Jared Simons
// Date Created: 5/6/2024
// 
// Purpose: The Interface for Event Systems.
//=============================================================================
#pragma once

enum class eEventCategory
{
	EVENT_TYPE_UNKNOWN = -1,
	EVENT_TYPE_WINDOW_RESIZE,
	EVENT_TYPE_APPLICATION_QUIT,
	EVENT_TYPE_INPUT_KEY_PRESSED,
	EVENT_TYPE_INPUT_KEY_RELEASED,
	EVENT_TYPE_INPUT_MOUSE_BPRESSED,
	EVENT_TYPE_INPUT_MOUSE_BRELEASED,
	EVENT_TYPE_INPUT_MOUSE_MOVED,
	
	EVENT_TYPE_MAX_EVENTS
};

typedef bool (*Event_Callback)(void* data);

class IEventSystem
{
public:
	virtual bool	EventSystem_Initialize() = 0;
	virtual void	EventSystem_Shutdown() = 0;

	virtual bool	EventRegister(eEventCategory eventCategory, Event_Callback PFN_Callback) = 0;
	virtual bool	EventUnregister(eEventCategory eventCategory, Event_Callback PFN_Callback) = 0;
	virtual bool	EventFire(eEventCategory eventCategory, void* eventData) = 0;
};

extern IEventSystem* gpEngineEvents;

