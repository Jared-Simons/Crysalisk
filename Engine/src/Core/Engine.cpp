//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "Engine.h"

#include "Core/Logging.h"
#include "Core/Event.h"
#include "Core/Input.h"
#include "Core/Memory.h"
#include "Platform/Platform.h"
#include "Systems/Renderer.h"
#include "Systems/TextureSystem.h"

#include <Memory/LinearAllocator.h>
#include <Memory/DynamicAllocator.h>

#include <UtilityLib_Core.h>
#include <Containers/CVector.h>
#include <Math/VertexTypes.h>

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

Engine* s_EnginePtr = nullptr;

bool Engine::Engine_Initialize(EngineConfig_t engineConfig, void* pEngineState)
{
	if (!pEngineState)
	{
		LOG_ERROR("Engine->Initialize() called without a pointer to an engine state. Application cannot continue.");
		return false;
	}

	Engine* typedState = reinterpret_cast<Engine*>(pEngineState);
	s_EnginePtr = typedState;

	s_EnginePtr->m_engineConfig = engineConfig;

	void* engineMemoryBlock = Platform::MemoryAllocate(MEBIBYTES(20));
	s_EnginePtr->m_EngineHeap = new DynamicAllocator(MEBIBYTES(20), engineMemoryBlock);

	// Logging Utilities
	LogSystem::Init(6); // Log all messages

	// Memory System
	if (!Memory::Initialize(s_EnginePtr->m_EngineHeap, s_EnginePtr->m_MemorySystem))
	{
		LOG_FATAL("Memory System failed to initialize! Application cannot continue.");
		return false;
	}

	ConnectUtilityLib(s_EnginePtr->m_EngineHeap, "Engine.dll");

	// Initialize the subsystem memory allocator
	s_EnginePtr->m_SystemsAllocator = new LinearAllocator(MEBIBYTES(1));
	LOG_INFO("Systems Allocator created with %u B", s_EnginePtr->m_SystemsAllocator->GetAllocatorSize());

	// Event System
	if (!EventSystem::Initialize())
	{
		LOG_FATAL("Event System failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events
	EventSystem::EventRegister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);
	EventSystem::EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	EventSystem::EventRegister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	EventSystem::EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	EventSystem::EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	EventSystem::EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);
	EventSystem::EventRegister(eEventCategory::WINDOW_START_RESIZE, nullptr, Event_WindowResizedPause);
	EventSystem::EventRegister(eEventCategory::WINDOW_END_RESIZE, nullptr, Event_WindowResizedUnpause);


	// Input System
	/*if (!IInputSystem::InputSystem_Initialize(gpInputSystem, m_SystemsAllocator))*/
	s_EnginePtr->m_InputSystem = IInputSystem::InputSystem_Initialize(s_EnginePtr->m_SystemsAllocator, nullptr);
	if (s_EnginePtr->m_InputSystem == nullptr)
	{
		LOG_FATAL("Input System failed to initialize! Application cannot continue.");
		return false;
	}

	// Platform Layer
	if (!s_EnginePtr->m_engineConfig.isHeadless)
	{
		if (!Platform::Initialize(s_EnginePtr->m_SystemsAllocator, s_EnginePtr))
		{
			LOG_FATAL("Platform Layer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Renderer
	if (!s_EnginePtr->m_engineConfig.isHeadless)
	{
		s_EnginePtr->m_RenderSystem = reinterpret_cast<Renderer*>(s_EnginePtr->m_SystemsAllocator->allocate(sizeof(Renderer)));
		if (!s_EnginePtr->m_RenderSystem->Initialize())
		{
			LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Texture System

	// TODO: Overloaded new operator?
	gpTextureSystem = (TextureSystem*)s_EnginePtr->m_SystemsAllocator->allocate(sizeof(TextureSystem));

	TextureSystemConfiguration textureSysConfig = { 1024 };
	if (!gpTextureSystem->Initialize(&textureSysConfig, s_EnginePtr->m_RenderSystem))
	{
		LOG_FATAL("Texture system failed to initialize! Application cannot continue.");
		return false;
	}

	// TEMP: Load up a texture
	gpTextureSystem->LoadTexture("Assets/Textures/plasterwall024b.png");

	typedState->testTexture = gpTextureSystem->AcquireTexture("Assets/Textures/plasterwall024b.png");

	// TEMP Geometry
	typedState->CreateSceneGeometry();

	// Initialize the frame allocator
	typedState->m_FrameAllocator = new LinearAllocator(MEBIBYTES(1));

	LOG_INFO("Engine Finished Initialization");
	return true;
}

bool Engine::Engine_Run()
{
	while (!s_bApplicationQuitCalled)
	{
		Platform::ProcessMessages();
		if (!g_sApplicationSuspended)
		{
			// Clear the frame allocator
			m_FrameAllocator->free();
			if (!m_engineConfig.isHeadless)
			{
				gpInputSystem->Update();

				Platform::Update();

				RenderPacket_t* frameRenderData = reinterpret_cast<RenderPacket_t*>(m_FrameAllocator->allocate(sizeof(RenderPacket_t)));
				frameRenderData->geometries[0] = m_TestGeometry;

				if (!s_EnginePtr->m_RenderSystem->Update(frameRenderData))
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

void Engine::Engine_Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	s_EnginePtr->m_RenderSystem->DestroyGeometryResources(*m_TestGeometry);
	gpTextureSystem->UnloadTexture(*testTexture);

	// TODO: Overloaded delete operator?
	gpTextureSystem->Shutdown();

	if (!m_engineConfig.isHeadless)
	{
		Renderer::Renderer_Shutdown(s_EnginePtr->m_RenderSystem, false);
	}

	if (!m_engineConfig.isHeadless)
	{
		Platform::Shutdown();
	}

	IInputSystem::InputSystem_Shutdown(gpInputSystem, false);

	// Unregister from engine-level events
	EventSystem::EventUnregister(eEventCategory::WINDOW_END_RESIZE, nullptr, Event_WindowResizedUnpause);
	EventSystem::EventUnregister(eEventCategory::WINDOW_START_RESIZE, nullptr, Event_WindowResizedPause);
	EventSystem::EventUnregister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);
	EventSystem::EventUnregister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	EventSystem::EventUnregister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	EventSystem::EventUnregister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	EventSystem::EventUnregister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	EventSystem::EventUnregister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);

	EventSystem::Shutdown();

	// Destroy allocators
	m_FrameAllocator->free(false);
	delete m_FrameAllocator;
	m_FrameAllocator = nullptr;

	m_SystemsAllocator->free(false);
	delete m_SystemsAllocator;
	m_SystemsAllocator = nullptr;

	DisconnectUtilityLib();

	Memory::Shutdown(m_MemorySystem);

	void* pEngineMemoryBlock = m_EngineHeap->GetMemoryBlock();
	delete m_EngineHeap;
	Platform::MemoryFree(pEngineMemoryBlock);

	LogSystem::Shutdown();
}

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
		Memory::PrintMemoryStats();
	}
	
	if (keycode == 'T')
	{
		// TODO: THIS CAUSES A MEMORY LEAK! TEXTURES ARE NOT BEING DESTROYED
		// Once this is properly implemented, we need to make sure this is no longer leaking resources.
		static int TextureChoice = 0;
		const char* textureNames[] =
		{
			"Assets/Textures/container.jpg",
			"Assets/Textures/plasterwall024b.png",
		};

		TextureChoice = TextureChoice == 0 ? 1 : 0;

		gpTextureSystem->LoadTexture(textureNames[TextureChoice]);
	}

	return true;
}

void Engine::CreateSceneGeometry()
{
	m_TestGeometry = reinterpret_cast<StaticMesh*>(Memory::Allocate(sizeof(StaticMesh), eMemAllocCategories::MEM_CATEGORY_ARRAY));

	CVector<Vertex3D> vertices(36);

	CVector<u16> indices((u16)0);

	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));

	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, -0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));

	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, -0.5f), Vector3D(0.0, 1.0, 0.0), Vector2D(1.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 0.0, 0.0), Vector2D(0.0f, 1.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(0.0, 0.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(0.5, 0.5, 0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(1.0f, 0.0f)));
	vertices.PushBack(Vertex3D(Vector3D(-0.5, 0.5, -0.5f), Vector3D(1.0, 1.0, 1.0), Vector2D(0.0f, 1.0f)));

	s_EnginePtr->m_RenderSystem->CreateGeometryResources(*m_TestGeometry, vertices, indices, "Assets/Textures/plasterwall024b.png", "TestGeometry");
}

bool Event_MouseButtonPressed(EventData_t data)
{
	u32 mouseButtonFlags = *data.u16;
	//LOG_DEBUG("%s Mouse Button Pressed", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}

bool Event_MouseButtonReleased(EventData_t data)
{
	u32 mouseButtonFlags = *data.u16;
	//LOG_DEBUG("%s Mouse Button Released", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}


static bool Event_WindowResized(EventData_t data)
{
	u32 width, height;
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

