//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "Systems/IEngine.h"

#include "Core/Logging.h"
#include "Systems/IMemory.h"
#include "Systems/IEvent.h"
#include "Systems/IInput.h"
#include "Platform/Platform.h"
#include "Systems/IRenderer.h"
#include "Systems/TextureSystem.h"

#include <Memory/LinearAllocator.h>
#include <Memory/DynamicAllocator.h>

static bool s_bApplicationQuitCalled = false;
static bool Event_ApplicationQuit(EventData_t data);
static bool Event_KeyPressed(EventData_t data);
static bool Event_KeyReleased(EventData_t data);
static bool Event_MouseButtonPressed(EventData_t data);
static bool Event_MouseButtonReleased(EventData_t data);
static bool Event_WindowResized(EventData_t data);
static bool Event_WindowResizedPause(EventData_t data);
static bool Event_WindowResizedUnpause(EventData_t data);

// TODO: Stick this in the engine class.
static bool g_sApplicationSuspended{ false };

class CEngine : public IEngine
{
public:
	bool	Engine_Initialize(EngineConfig_t engineConfig) override;

	bool	Engine_Run() override;

	void	Engine_Shutdown() override;

private:
	EngineConfig_t	m_engineConfig;
	DynamicAllocator* m_EngineHeap{ nullptr };
	LinearAllocator* m_SystemsAllocator{ nullptr };
};

bool CEngine::Engine_Initialize(EngineConfig_t engineConfig)
{
	m_engineConfig = engineConfig;

	void* engineMemoryBlock = Platform::MemoryAllocate(MEBIBYTES(20));
	m_EngineHeap = new DynamicAllocator(MEBIBYTES(20), engineMemoryBlock);

	// Logging Utilities
	Logging_Initialize();

	// Memory System
	if (!gpMemorySystem->MemorySystem_Initialize(*m_EngineHeap))
	{
		LOG_FATAL("Memory System failed to initialize! Application cannot continue.");
		return false;
	}

	// Initialize the subsystem memory allocator
	m_SystemsAllocator = new LinearAllocator(MEBIBYTES(1));
	LOG_INFO("Systems Allocator created with %u B", m_SystemsAllocator->GetAllocatorSize());

	// Event System
	if (!IEventSystem::EventSystem_Create(gpEngineEvents, m_SystemsAllocator))
	{
		LOG_FATAL("Event System failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, Event_KeyPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, Event_KeyReleased);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, Event_MouseButtonPressed);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, Event_MouseButtonReleased);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, Event_WindowResized);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_WINDOW_START_RESIZE, Event_WindowResizedPause);
	gpEngineEvents->EventRegister(eEventCategory::EVENT_TYPE_WINDOW_END_RESIZE, Event_WindowResizedUnpause);


	// Input System
	if (!IInputSystem::InputSystem_Initialize(gpInputSystem, m_SystemsAllocator))
	{
		LOG_FATAL("Input System failed to initialize! Application cannot continue.");
		return false;
	}

	// Platform Layer
	if (!m_engineConfig.isHeadless)
	{
		if (!Platform::Initialize(m_SystemsAllocator))
		{
			LOG_FATAL("Platform Layer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Renderer
	if (!m_engineConfig.isHeadless)
	{
		if (!IRenderer::Renderer_Create(gpRenderer, m_SystemsAllocator))
		{
			LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Texture System

	// TODO: Overloaded new operator?
	gpTextureSystem = (TextureSystem*)m_SystemsAllocator->allocate(sizeof(TextureSystem));

	TextureSystemConfiguration textureSysConfig = { 1024 };
	if (!gpTextureSystem->Initialize(&textureSysConfig))
	{
		LOG_FATAL("Texture system failed to initialize! Application cannot continue.");
		return false;
	}

	// TEMP: Load up a texture
	gpTextureSystem->LoadTexture("Assets/Textures/container.jpg");

	LOG_INFO("Engine Finished Initialization");
	return true;
}

bool CEngine::Engine_Run()
{
	while (!s_bApplicationQuitCalled)
	{
		Platform::ProcessMessages();
		if (!g_sApplicationSuspended)
		{
			if (!m_engineConfig.isHeadless)
			{
				gpInputSystem->Update();

				Platform::Update();

				if (!gpRenderer->Update())
				{
					LOG_FATAL("Renderer update failed!");
					return false;
				}
			}
		}

		return true;
	}

	Engine_Shutdown();

	// Returning false signifies that the engine is done running and ready for shutdown
	return false;
}

void CEngine::Engine_Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	// TODO: Overloaded delete operator?
	gpTextureSystem->Shutdown();

	if (!m_engineConfig.isHeadless)
	{
		IRenderer::Renderer_Shutdown(gpRenderer, false);
	}

	if (!m_engineConfig.isHeadless)
	{
		Platform::Shutdown();
	}

	IInputSystem::InputSystem_Shutdown(gpInputSystem, false);

	// Unregister from engine-level events
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_WINDOW_END_RESIZE, Event_WindowResizedUnpause);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_WINDOW_START_RESIZE, Event_WindowResizedPause);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_WINDOW_RESIZE, Event_WindowResized);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BRELEASED, Event_MouseButtonReleased);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_MOUSE_BPRESSED, Event_MouseButtonPressed);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_KEY_RELEASED, Event_KeyReleased);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_INPUT_KEY_PRESSED, Event_KeyPressed);
	gpEngineEvents->EventUnregister(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, Event_ApplicationQuit);

	IEventSystem::EventSystem_Shutdown(gpEngineEvents, false);

	// Destroy the subsystem allocator
	m_SystemsAllocator->free(false);
	delete m_SystemsAllocator;
	m_SystemsAllocator = nullptr;

	gpMemorySystem->MemorySystem_Shutdown();

	void* pEngineMemoryBlock = m_EngineHeap->GetMemoryBlock();
	delete m_EngineHeap;
	Platform::MemoryFree(pEngineMemoryBlock);

	Logging_Shutdown();
}

static CEngine g_Engine;
IEngine* Engine = &g_Engine;


bool Event_ApplicationQuit(EventData_t data)
{
	LOG_VERBOSE("Application_Quit() called");
	s_bApplicationQuitCalled = true;
	return true;
}

bool Event_KeyPressed(EventData_t data)
{
	char keycode = *data.c;
	//LOG_DEBUG("%c Key Pressed", keycode);
	return true;
}

bool Event_KeyReleased(EventData_t data)
{
	char keycode = *data.c;
	//LOG_DEBUG("%c Key Released", keycode);

	if (keycode == 'M')
	{
		gpMemorySystem->PrintMemoryStats();
	}
	
	if (keycode == 'T')
	{
		// TODO: THIS CAUSES A MEMORY LEAK! TEXTURES ARE NOT BEING DESTROYED
		// Once this is properly implemented, we need to make sure this is no longer leaking resources.
		static int TextureChoice = 0;
		const char* textureNames[] =
		{
			"Assets/Textures/container.jpg",
			"Assets/Textures/container2.png",
		};

		TextureChoice = TextureChoice == 0 ? 1 : 0;

		gpTextureSystem->LoadTexture(textureNames[TextureChoice]);
	}

	return true;
}

bool Event_MouseButtonPressed(EventData_t data)
{
	unsigned int mouseButtonFlags = *data.u16;
	//LOG_DEBUG("%s Mouse Button Pressed", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}

bool Event_MouseButtonReleased(EventData_t data)
{
	unsigned int mouseButtonFlags = *data.u16;
	//LOG_DEBUG("%s Mouse Button Released", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}


static bool Event_WindowResized(EventData_t data)
{
	unsigned int width, height;
	width = data.u16[0];
	height = data.u16[1];

	LOG_DEBUG("Window Resized: %d / %d", width, height);

	if (width == 0 || height == 0)
	{
		g_sApplicationSuspended = true;
	}
	else
	{
		g_sApplicationSuspended = false;
	}

	return true;
}

static bool Event_WindowResizedPause(EventData_t data)
{
	g_sApplicationSuspended = true;
	return true;
}

static bool Event_WindowResizedUnpause(EventData_t data)
{
	g_sApplicationSuspended = false;
	return true;
}

