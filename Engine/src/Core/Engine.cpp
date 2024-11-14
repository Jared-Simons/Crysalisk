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
	u64 memorySystemMemoryRequirement = gpMemory->GetSystemMemoryRequirement();
	void* pMemorySysBlock = s_EnginePtr->m_EngineHeap->Allocate(memorySystemMemoryRequirement);
	gpMemory = gpMemory->Create(pMemorySysBlock);
	if (!gpMemory->Initialize(s_EnginePtr->m_EngineHeap))
	{
		LOG_FATAL("Memory System failed to initialize! Application cannot continue.");
		return false;
	}

	ConnectUtilityLib(s_EnginePtr->m_EngineHeap, "Engine.dll");

	// Initialize the subsystem memory allocator
	s_EnginePtr->m_SystemsAllocator = new LinearAllocator(MEBIBYTES(1));
	LOG_INFO("Systems Allocator created with %u B", s_EnginePtr->m_SystemsAllocator->GetAllocatorSize());

	// Event System
	gpEvents = IEventSystem::Create(s_EnginePtr->m_SystemsAllocator);
	
	if (gpEvents == nullptr)
	{
		LOG_FATAL("Event System failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events
	gpEvents->EventRegister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);
	gpEvents->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	gpEvents->EventRegister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	gpEvents->EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	gpEvents->EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	gpEvents->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);
	gpEvents->EventRegister(eEventCategory::WINDOW_START_RESIZE, nullptr, Event_WindowResizedPause);
	gpEvents->EventRegister(eEventCategory::WINDOW_END_RESIZE, nullptr, Event_WindowResizedUnpause);


	// Input System
	gpInputSystem = IInputSystem::InputSystem_Initialize(s_EnginePtr->m_SystemsAllocator);
	if (gpInputSystem == nullptr)
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
		gpRenderer = Renderer::Renderer_Create(s_EnginePtr->m_SystemsAllocator);
		if (!gpRenderer)
		{
			LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
			return false;
		}
	}

	// Texture System

	// TODO: Overloaded new operator?
	gpTextureSystem = (TextureSystem*)s_EnginePtr->m_SystemsAllocator->Allocate(sizeof(TextureSystem));

	TextureSystemConfiguration textureSysConfig = { 1024 };
	if (!gpTextureSystem->Initialize(&textureSysConfig))
	{
		LOG_FATAL("Texture system failed to initialize! Application cannot continue.");
		return false;
	}

	// TEMP: Load up a texture
	gpTextureSystem->LoadTexture("Assets/Textures/container.jpg");

	//typedState->testTexture = gpTextureSystem->AcquireTexture("Assets/Textures/plasterwall024b.png");

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
			m_FrameAllocator->Free();
			if (!m_engineConfig.isHeadless)
			{
				gpInputSystem->Update();

				Platform::Update();

				RenderPacket_t* frameRenderData = reinterpret_cast<RenderPacket_t*>(m_FrameAllocator->Allocate(sizeof(RenderPacket_t)));
				frameRenderData->geometries[0] = m_TestGeometry;

				if (!gpRenderer->Update(frameRenderData))
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

	gpRenderer->DestroyGeometryResources(*m_TestGeometry);
	//gpTextureSystem->UnloadTexture(*testTexture);

	// TODO: Overloaded delete operator?
	gpTextureSystem->Shutdown();

	if (!m_engineConfig.isHeadless)
	{
		gpRenderer->Shutdown();
	}

	if (!m_engineConfig.isHeadless)
	{
		Platform::Shutdown();
	}

	gpInputSystem->Shutdown();

	// Unregister from engine-level events
	gpEvents->EventUnregister(eEventCategory::WINDOW_END_RESIZE, nullptr, Event_WindowResizedUnpause);
	gpEvents->EventUnregister(eEventCategory::WINDOW_START_RESIZE, nullptr, Event_WindowResizedPause);
	gpEvents->EventUnregister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);
	gpEvents->EventUnregister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	gpEvents->EventUnregister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	gpEvents->EventUnregister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	gpEvents->EventUnregister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	gpEvents->EventUnregister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);

	gpEvents->Shutdown();

	// Destroy allocators
	m_FrameAllocator->Free(false);
	delete m_FrameAllocator;
	m_FrameAllocator = nullptr;

	m_SystemsAllocator->Free(false);
	delete m_SystemsAllocator;
	m_SystemsAllocator = nullptr;

	DisconnectUtilityLib();

	gpMemory->Shutdown(gpMemory);

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
		gpMemory->PrintMemoryStats();
	}
	
	if (keycode == 'T')
	{
		Texture* pOldTexture = s_EnginePtr->m_TestGeometry->m_pTexture;

		// TODO: THIS CAUSES A MEMORY LEAK! TEXTURES ARE NOT BEING DESTROYED
		// Once this is properly implemented, we need to make sure this is no longer leaking resources.
		static i32 TextureChoice = 0;
		const char* textureNames[] =
		{
			"Assets/Textures/container.jpg",
			"Assets/Textures/container2.png",
			//"Assets/Textures/plasterwall024b.png",
		};

		TextureChoice = TextureChoice == 0 ? 1 : 0;

		// Load up the new texture.
		s_EnginePtr->m_TestGeometry->m_pTexture = gpTextureSystem->AcquireTexture(textureNames[TextureChoice]);

		gpTextureSystem->UnloadTexture(*pOldTexture);
	}

	return true;
}

void Engine::CreateSceneGeometry()
{
	m_TestGeometry = reinterpret_cast<StaticMesh*>(gpMemory->Allocate(sizeof(StaticMesh), eMemAllocCategories::MEM_CATEGORY_ARRAY));

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

	gpRenderer->CreateGeometryResources(*m_TestGeometry, vertices, indices, "Assets/Textures/container.jpg", "TestGeometry");
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

