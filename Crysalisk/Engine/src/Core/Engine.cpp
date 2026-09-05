//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================
#include "Engine.h"

#include "Application.h"
#include <Core/ConVar.h>
#include <Core/Event.h>
#include <Core/Logging.h>
#include <Math/Ray.h>
#include <Memory/MemoryManager.h>
#include <Platform/Platform.h>
#include "Renderer/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererTypes.h"
#include "Resources/Map.h"
#include "Systems/DebugGeometrySystem.h"
#include "Systems/FontSystem.h"
#include "Systems/JobSystem.h"
#include "Systems/MaterialSystem.h"
#include "Systems/MeshSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"

static bool s_bApplicationQuitCalled{ false };
static bool s_bApplicationSuspended{ false };
bool		s_bLevelGeometryLoaded{ false };

// Function defined in user code to setup application state.
extern bool CreateApplication(ApplicationConfig_t* pAppConfig);

// Engine Event Handlers
static bool Event_ApplicationQuit(void* pEventListener, EventData_t data);
static bool Event_KeyPressed(void* pEventListener, EventData_t data);
static bool Event_KeyReleased(void* pEventListener, EventData_t data);
static bool Event_MouseButtonPressed(void* pEventListener, EventData_t data);
static bool Event_MouseButtonReleased(void* pEventListener, EventData_t data);
static bool Event_MouseMoved(void* pEventListener, EventData_t data);
static bool Event_WindowResized(void* pEventListener, EventData_t data);
static bool Event_WindowResizedPause(void* pEventListener, EventData_t data);
static bool Event_WindowResizedUnpause(void* pEventListener, EventData_t data);

// Console Command callbacks.
static void ConsoleCommand_LoadMap(struct ConVar_t* pCommand, void* pValue);

//-------------------------------------------------------------------------
// Implementation of the core engine class.
//-------------------------------------------------------------------------
class CEngine : public IEngine
{
public:
	virtual bool	Initialize(EngineConfig_t engineConfig) override;
	virtual bool	Run() override;
	virtual void	Shutdown() override;

	virtual Platform::IPlatformWindow*	GetEngineWindow() const override { return m_pEngineWindow; }
	virtual ICrysApplication*			GetApplication() const override { return m_pApplication; }

	virtual inline const EngineInterfaces_t*		GetEngineInterfaces() const override { return &m_EngineInterfaces; }

	const CMap&		GetCurrentMap() const { return m_CurrentMap; }

private:
	EngineConfig_t		m_engineConfig;
	ICrysApplication*	m_pApplication{ nullptr };

	EngineInterfaces_t		m_EngineInterfaces{};

	Platform::IPlatformWindow* m_pEngineWindow = nullptr;
	RenderPacket_t				m_FrameRenderData{};

	CMap			m_CurrentMap{};

	Clock		m_UpdateClock{};
	f64			m_LastUpdateTime{ 0.0f };

	// These need access to our members.
	friend static bool Event_KeyReleased(void* pEventListener, EventData_t data);
	friend static bool Event_MouseButtonReleased(void* pEventListener, EventData_t data);
	friend bool CreateEngineInstance(struct ApplicationConfig_t* pAppConfig);
};

// Engine context.
static CEngine s_Engine;
IEngine* g_pEngine = &s_Engine;

//-------------------------------------------------------------------------
// Purpose: Standup base engine state and systems.
// Input:	pAppConfig - A configuration passed in by the calling application.
// Output:	bool - Returns true on successful setup, otherwise false.
//-------------------------------------------------------------------------
bool CreateEngineInstance(struct ApplicationConfig_t* pAppConfig)
{
	pAppConfig->pEngine = g_pEngine;
	CAssert(pAppConfig->pEngine);

	CEngine* pEngine = (CEngine*)pAppConfig->pEngine;

	// Stand up the memory manager.
	ApplicationMemoryConfiguration_t memoryConfig{};
	//memoryConfig.totalMemoryBudget = MEBIBYTES(1000);
	memoryConfig.totalMemoryBudget = GIBIBYTES(2ULL);
	if (!IMemoryManager::CreateMemorySystem(memoryConfig))
	{
		// This error is handled by a platform-specific message box, as the logging system technically isn't set up yet.
		Platform::DisplayMessageBox(nullptr, "Memory manager failed to initialize! Application cannot continue.", "Engine Error", false);
		return false;
	}

	// Logger.
	pEngine->m_EngineInterfaces.pLogger = new LogSystem;
	g_pLog = pEngine->m_EngineInterfaces.pLogger;	// We need this global var for our logging macros.
	if (!pEngine->m_EngineInterfaces.pLogger->Init((i32)eLogLevel::LOG_LEVEL_VERBOSE)) // Log all messages
	{
		// This error is handled by a platform-specific message box, as the logging system technically failed to initialize.
		Platform::DisplayMessageBox(nullptr, "Logger failed to initialize! Application cannot continue.", "Engine Error", false);
		return false;
	}

	// ConVar system.
	AllocateConVarSystemState(&pEngine->m_EngineInterfaces.pConVar);
	if (!InitializeConVarSystem(pEngine->m_EngineInterfaces.pConVar))
	{
		LOG_ERROR("ConVar system failed to initialize! Application cannot continue.");
		return false;
	}

	// Event System.
	IEventSystem::AllocateEventSystemState(&pEngine->m_EngineInterfaces.pEngineEventHandler);
	if (!pEngine->m_EngineInterfaces.pEngineEventHandler->Initialize())
	{
		LOG_FATAL("Event system failed to initialize! Application cannot continue.");
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Initialize the engine and it's required subsystems.
// Input:	engineConfig - A configuration passed in by the calling application.
// Output:	bool - Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool CEngine::Initialize(EngineConfig_t engineConfig)
{
	Clock initializationClock{};
	initializationClock.Start();

	m_engineConfig = engineConfig;
	
	// NOTE: Setting the map here so that external applications can access it.
	m_EngineInterfaces.pCurrentScene = &m_CurrentMap;

	// Let the application set up some state.
	if (!engineConfig.pAppConfig->pApplication)
	{
		LOG_FATAL("Engine was not provided with an application!");
		return false;
	}
	m_pApplication = engineConfig.pAppConfig->pApplication;
	m_pApplication->PreInitialize(engineConfig.pAppConfig);

	// Platform layer.
	if (!Platform::InitializePlatformLayer())
	{
		LOG_FATAL("Failed to initialize platform layer! Application cannot continue.");
		return false;
	}

	// Setup the main engine window.
	Platform::WindowParams_t windowParams{};
	windowParams.pszName = "Crysalisk Engine\0";
	windowParams.X = 100;
	windowParams.Y = 100;
	windowParams.Width = 1080;
	windowParams.Height = 720;
	m_pEngineWindow = Platform::CreatePlatformWindow(windowParams, true);
	m_pEngineWindow->SetWindowResizeCallback(WindowResizeHandler);
	m_pEngineWindow->SetWindowCloseCallback(WindowClosedHandler);

	// Load up external libraries now, so they can get interfaces prior to initialization.
	Platform::ISysModule* pRendererModule = Platform::LoadSysModule("Renderer_Vulkan.dll");
	if (!pRendererModule)
	{
		LOG_FATAL("Failed to load Renderer_Vulkan.dll! Application cannot continue.");
		return false;
	}

	// Input System.
	IInputSystem::AllocateInputSystemState(&m_EngineInterfaces.pInputSystem);
	if (!m_EngineInterfaces.pInputSystem->Initialize(*m_pEngineWindow, m_EngineInterfaces.pEngineEventHandler))
	{
		LOG_FATAL("Input system failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events.
	IEventSystem* pEventHandler = m_EngineInterfaces.pEngineEventHandler;
	pEventHandler->EventRegister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);
	pEventHandler->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);
	pEventHandler->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	pEventHandler->EventRegister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	pEventHandler->EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	pEventHandler->EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	pEventHandler->EventRegister(eEventCategory::INPUT_MOUSE_MOVED, nullptr, Event_MouseMoved);

	// Renderer.
	RendererConfig_t rendererConfig{};
	rendererConfig.szApplicationName = "Crysalisk Engine";
	rendererConfig.pEngine = this;
	rendererConfig.pMainWindow = m_pEngineWindow;
	rendererConfig.pRendererBackendDLL = pRendererModule;
	rendererConfig.MaxTextures = 4999;
	rendererConfig.MaxPipelines = 300;

	IRenderer::AllocateRenderSystemState(&m_EngineInterfaces.pRenderer);
	if (!m_EngineInterfaces.pRenderer->Initialize(rendererConfig))
	{
		LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
		return false;
	}

	// Job system.
	JobSystemConfig_t jobSysConfig{};
	jobSysConfig.iNumJobThreads = Platform::GetNumberOfSystemProcessors() - 1;	// - 1 to account for the main thread.

	// NOTE: Currently, we will just use one thread for resource loading.
	for (i32 iThread = 0; iThread < jobSysConfig.iNumJobThreads; iThread++)
	{
		jobSysConfig.iWorkerThreadTypes[iThread] = WORKER_THREAD_TYPE_GENERAL;
	}
	jobSysConfig.iWorkerThreadTypes[0] = WORKER_THREAD_TYPE_DISK_LOADING;

	IJobSystem::AllocateJobSystemState(&m_EngineInterfaces.pJobSystem);
	if (!m_EngineInterfaces.pJobSystem->Initialize(jobSysConfig))
	{
		LOG_FATAL("Failed to initialize job system! Application cannot continue.");
		return false;
	}

	// Save off a pointer to the renderer library so apps can have it.
	engineConfig.pAppConfig->pRendererLibrary = pRendererModule;

	// Texture manager.
	TextureSystemConfig_t textureSysConfig{};
	textureSysConfig.MaxTextures = 4999;
	textureSysConfig.pRenderer = m_EngineInterfaces.pRenderer;
	textureSysConfig.pJobSystem = m_EngineInterfaces.pJobSystem;
	CAssert(rendererConfig.MaxTextures == textureSysConfig.MaxTextures);

	ITextureSystem::AllocateTextureSystemState(&m_EngineInterfaces.pTextureSystem);
	if (!m_EngineInterfaces.pTextureSystem->Initialize(textureSysConfig))
	{
		LOG_ERROR("Failed to initialize texture system! Application cannot continue.");
		return false;
	}

	// Shader system.
	ShaderSystemConfig_t shaderSysConfig{};
	shaderSysConfig.MaxShaders = 197;
	shaderSysConfig.pRenderer = m_EngineInterfaces.pRenderer;

	IShaderSystem::AllocateShaderSystemState(&m_EngineInterfaces.pShaderSystem);
	if (!m_EngineInterfaces.pShaderSystem->Initialize(shaderSysConfig))
	{
		LOG_ERROR("Failed to initialize shader system! Application cannot continue.");
		return false;
	}

	// Debug renderer.
	DebugRendererConfig_t debugRendererConfig{};
	debugRendererConfig.uMaxDebugGeometries = 4096;
	debugRendererConfig.pRenderer = m_EngineInterfaces.pRenderer;
	debugRendererConfig.pShaderSystem = m_EngineInterfaces.pShaderSystem;
	
	IDebugGeometrySystem::AllocateDebugGeometrySystemState(&m_EngineInterfaces.pDebugGeometry);
	if (!m_EngineInterfaces.pDebugGeometry->Initialize(debugRendererConfig))
	{
		LOG_ERROR("Failed to initialize the debug geometry system.");
		return false;
	}

	// Material system.
	MaterialSystemConfig_t materialSysConfig{};
	materialSysConfig.MaxMaterials = 2049;
	materialSysConfig.pTextureSystem = m_EngineInterfaces.pTextureSystem;
	materialSysConfig.pShaderSystem = m_EngineInterfaces.pShaderSystem;

	IMaterialSystem::AllocateMaterialSystemState(&m_EngineInterfaces.pMaterialSystem);
	if (!m_EngineInterfaces.pMaterialSystem->Initialize(materialSysConfig))
	{
		LOG_ERROR("Failed to initialize material system! Application cannot continue.");
		return false;
	}

	// Mesh system.
	MeshSystemConfig_t meshSysConfig{};
	meshSysConfig.MaxMeshes = 249;
	meshSysConfig.pRenderer = m_EngineInterfaces.pRenderer;
	meshSysConfig.pJobSystem = m_EngineInterfaces.pJobSystem;
	meshSysConfig.pMaterialSystem = m_EngineInterfaces.pMaterialSystem;

	IMeshSystem::AllocateMeshSystemState(&m_EngineInterfaces.pMeshSystem);
	if (!m_EngineInterfaces.pMeshSystem->Initialize(meshSysConfig))
	{
		LOG_ERROR("Failed to initialize mesh system! Application cannot continue.");
		return false;
	}

	// Font system.
	FontSystemConfig_t fontSysConfig{};
	fontSysConfig.MaxFonts = 19;
	fontSysConfig.pRenderer = m_EngineInterfaces.pRenderer;
	fontSysConfig.pTextureSystem = m_EngineInterfaces.pTextureSystem;
	fontSysConfig.pShaderSystem = m_EngineInterfaces.pShaderSystem;

	IFontSystem::AllocateFontSystemState(&m_EngineInterfaces.pFontSystem);
	if (!m_EngineInterfaces.pFontSystem->Initialize(fontSysConfig))
	{
		LOG_ERROR("Failed to initialize font system! Application cannot continue.");
		return false;
	}

	// Setup application-specific renderer state.
	m_EngineInterfaces.pRenderer->ConfigureRenderViews(engineConfig.pAppConfig->pViews);

	// Run Post-Engine initialize routines.
	m_EngineInterfaces.pRenderer->PostInitialize(m_FrameRenderData);
	m_EngineInterfaces.pMaterialSystem->PostInitialize(&m_EngineInterfaces);
	m_EngineInterfaces.pFontSystem->PostInitialize();

	// Initialize the application.
	engineConfig.pAppConfig->pEngine = this;
	m_pApplication->Initialize(engineConfig.pAppConfig);

	// TODO: This is bad.
	if (engineConfig.pAppConfig->pWindowToRegisterWithRenderer)
	{
		m_EngineInterfaces.pRenderer->RegisterWindow(*engineConfig.pAppConfig->pWindowToRegisterWithRenderer);
	}
	else
	{
		// Register the game window with the renderer.
		m_EngineInterfaces.pRenderer->RegisterWindow(*m_pEngineWindow);
	}

	// Print memory state
	g_pMemoryManager->PrintMemoryStats();

	initializationClock.Update();
	LOG_INFO("Engine Finished Initialization, initialization took: %f s", initializationClock.GetElapsedTime());

	LOG_DEBUG("Memory usage post-initialization: %.5f", g_pMemoryManager->GetPercentAllocated());

	// Register engine console commands.
	ConVarSystemState* pConVar = m_EngineInterfaces.pConVar;
	ConsoleCommandRegister(pConVar, "map", CONVAR_VALUETYPE_PASSTHROUGH, ConsoleCommand_LoadMap, this);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Runs a single engine frame.
// Input:	none.
// Output:	bool - Returns true on successful frame, otherwise false.
//-------------------------------------------------------------------------
bool CEngine::Run()
{
	m_UpdateClock.Start();

	m_EngineInterfaces.pDebugGeometry->AddText(m_EngineInterfaces.pFontSystem, Vector{ 815.0f, 238.0f, -224.0f }, 0, "*  THIS IS A BAR!", Vector4D{ 1.0f, 0.8f, 0.0f, 1.0f }, - 1.0f);
	while (!s_bApplicationQuitCalled)
	{
		Platform::ProcessMessages();
		// NOTE: We need to check for the application quit event again here, because it might've been set during 'Platform::ProcessMessages()' 
		// and trying to run another frame may result in a fatal crash (the renderer attempting to render to a window that no longer exists, for example).
		if (!s_bApplicationSuspended && !s_bApplicationQuitCalled)
		{
			// Calculate our update delta time.
			f64 currentFrameTime = m_UpdateClock.GetElapsedTime();
			f64 deltaTime = currentFrameTime - m_LastUpdateTime;
			m_LastUpdateTime = currentFrameTime;

			// Update our app.
			if (!m_pApplication->Run(deltaTime))
			{
				// Either the application failed, or it requested to shutdown.
				return true;
			}

			// Handle jobs.
			m_EngineInterfaces.pJobSystem->Update();

			m_FrameRenderData.DeltaTime = deltaTime;

			// Get render data from the application.
			m_pApplication->FramePrepare(&m_FrameRenderData);

			// Run the app's render routine.
			if (!m_pApplication->Render(&m_FrameRenderData))
			{
				// App failed to render.
				return false;
			}

			m_EngineInterfaces.pRenderer->Update(m_pEngineWindow, m_FrameRenderData, &m_CurrentMap);

			m_EngineInterfaces.pInputSystem->Update();
		}
	}

	for (i32 iView = 0; iView < RENDER_VIEW_MAX_TYPES; iView++)
	{
		if (m_FrameRenderData.pRenderViewExtendedData[iView])
		{
			delete m_FrameRenderData.pRenderViewExtendedData[iView];
			m_FrameRenderData.pRenderViewExtendedData[iView] = nullptr;
		}
	}

	Shutdown();
	return true;
}

//-------------------------------------------------------------------------
// Purpose:	Shutdows down the engine.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CEngine::Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	// Wait for threads to be finished.
	m_EngineInterfaces.pJobSystem->WaitForWorkerThreadsToFinish();

	// Shutdown our app.
	m_pApplication->Shutdown();

	if (m_CurrentMap.IsMapLoaded())
		m_CurrentMap.UnloadMap(m_EngineInterfaces.pMeshSystem);

	m_EngineInterfaces.pFontSystem->Shutdown();
	delete m_EngineInterfaces.pFontSystem;
	m_EngineInterfaces.pFontSystem = nullptr;

	m_EngineInterfaces.pMeshSystem->Shutdown();
	delete m_EngineInterfaces.pMeshSystem;
	m_EngineInterfaces.pMeshSystem = nullptr;

	m_EngineInterfaces.pMaterialSystem->Shutdown();
	delete m_EngineInterfaces.pMaterialSystem;
	m_EngineInterfaces.pMaterialSystem = nullptr;

	m_EngineInterfaces.pDebugGeometry->Shutdown();
	delete m_EngineInterfaces.pDebugGeometry;
	m_EngineInterfaces.pDebugGeometry = nullptr;

	m_EngineInterfaces.pShaderSystem->Shutdown();
	delete m_EngineInterfaces.pShaderSystem;
	m_EngineInterfaces.pShaderSystem = nullptr;

	m_EngineInterfaces.pTextureSystem->Shutdown();
	delete m_EngineInterfaces.pTextureSystem;
	m_EngineInterfaces.pTextureSystem = nullptr;

	m_EngineInterfaces.pJobSystem->Shutdown();
	delete m_EngineInterfaces.pJobSystem;
	m_EngineInterfaces.pJobSystem = nullptr;

	// Release extra window resources.
	if (m_engineConfig.pAppConfig->pWindowToRegisterWithRenderer)
	{
		m_EngineInterfaces.pRenderer->ReleaseWindowResources(*m_engineConfig.pAppConfig->pWindowToRegisterWithRenderer);
		delete m_engineConfig.pAppConfig->pWindowToRegisterWithRenderer;
		m_engineConfig.pAppConfig->pWindowToRegisterWithRenderer = nullptr;
	}

	m_EngineInterfaces.pRenderer->Shutdown();
	delete m_EngineInterfaces.pRenderer;
	m_EngineInterfaces.pRenderer = nullptr;

	// Release our main window.
	if (m_pEngineWindow)
	{
		delete m_pEngineWindow;
		m_pEngineWindow = nullptr;
	}

	Platform::ShutdownPlatformLayer();

	m_EngineInterfaces.pInputSystem->Shutdown();
	delete m_EngineInterfaces.pInputSystem;
	m_EngineInterfaces.pInputSystem = nullptr;

	m_EngineInterfaces.pEngineEventHandler->Shutdown();
	delete m_EngineInterfaces.pEngineEventHandler;
	m_EngineInterfaces.pEngineEventHandler = nullptr;

	ShutdownConVarSystem(m_EngineInterfaces.pConVar);
	delete m_EngineInterfaces.pConVar;
	m_EngineInterfaces.pConVar = nullptr;
	
	m_EngineInterfaces.pLogger->Shutdown();
	delete m_EngineInterfaces.pLogger;
	m_EngineInterfaces.pLogger = nullptr;

	g_pMemoryManager->Shutdown();
}

//-------------------------------------------------------------------------
// Engine-level event handlers.
//-------------------------------------------------------------------------
bool Event_ApplicationQuit(void* pEventListener, EventData_t data)
{
	LOG_VERBOSE("Application_Quit() called");
	s_bApplicationQuitCalled = true;
	return true;
}

bool Event_KeyPressed(void* pEventListener, EventData_t data)
{
	char keycode = *data.c;
	//if (keycode >= 65 && keycode <= 90)
		//LOG_DEBUG("%c Key Pressed", keycode);
	return true;
}

bool Event_KeyReleased(void* pEventListener, EventData_t data)
{
	char keycode = *data.c;
	//if (keycode >= 65 && keycode <= 90)
		//LOG_DEBUG("%c Key Released", keycode);

	if (keycode == KEY_ESCAPE)
	{
		// Time to shutdown the application!
		s_Engine.GetEngineInterfaces()->pEngineEventHandler->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
	}

	if (keycode == 'M')
	{
		g_pMemoryManager->PrintMemoryStats();
	}
	else if (keycode == 'L')
	{
		// FIXME: Remove this once the TestApp/game code has access to the developer console.
		Clock mapLoadTimer{};
		mapLoadTimer.Start();
		if (!s_Engine.m_CurrentMap.IsMapLoaded())
		{
			LoadMap("TestMap", s_Engine.GetEngineInterfaces()->pMeshSystem, s_Engine.GetEngineInterfaces()->pMaterialSystem, s_Engine.m_CurrentMap);
			LOG_DEBUG("Memory usage post-map resource free: %.10f", g_pMemoryManager->GetPercentAllocated());
			LOG_WARN("Map load took %.5f", mapLoadTimer.GetElapsedTime());

			s_Engine.m_EngineInterfaces.pCurrentScene = &s_Engine.m_CurrentMap;
		}
	}
	else if (keycode == 'U')
	{
		if (s_Engine.m_CurrentMap.IsMapLoaded())
		{
			s_Engine.m_CurrentMap.UnloadMap(s_Engine.m_EngineInterfaces.pMeshSystem);
			LOG_DEBUG("Memory usage post-map unload: %.10f", g_pMemoryManager->GetPercentAllocated());
		}
	}

	return true;
}

bool Event_MouseButtonPressed(void* pEventListener, EventData_t data)
{
	u32 mouseButtonFlags = *data.u16;
	//LOG_DEBUG("%s Mouse Button Pressed", e_MouseButtonTable_Strings[mouseButtonFlags]);
	return true;
}

bool Event_MouseButtonReleased(void* pEventListener, EventData_t data)
{
	/*u32 mouseButtonFlags = *data.u16;
	if (mouseButtonFlags == MOUSE_BUTTON_LEFT)
	{
		i32 iMouseX{ 0 }, iMouseY{ 0 };
		s_Engine.GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(iMouseX, iMouseY);

		Ray_t ray = GetRayFromScreen(s_Engine.GetEngineWindow(), s_Engine.GetApplication()->GetMainCamera(), iMouseX, iMouseY);

		// We can only actually cast the ray if the level is loaded.
		if (s_Engine.GetCurrentMap().IsMapLoaded())
		{
			s_Engine.GetCurrentMap().CastRayIntoScene(ray, s_Engine.GetEngineInterfaces()->pDebugGeometry);
			Vector4D rayColor{};
			if (ray.bDidHit)
			{
				LOG_DEBUG("HIT");
				rayColor = { 0.0f, 1.0f, 0.0f, 1.0f };
			}
			else
			{
				LOG_DEBUG("MISS");
				rayColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			}
			s_Engine.GetEngineInterfaces()->pDebugGeometry->AddPoint(ray.vecEnd, 30.0f, Vector4D{ 1.0f, 0.0f, 0.0f, 1.0f }, 5.0f);
			s_Engine.GetEngineInterfaces()->pDebugGeometry->AddLine(ray.vecOrigin, ray.vecEnd, rayColor, 5.0f);
			
			if (ray.pHitGeometry)
			{
				s_Engine.GetEngineInterfaces()->pDebugGeometry->AddBBox(*ray.pHitGeometry, Vector4D{ 0.0f, 0.0f, 1.0f, 1.0f }, 5.0f);
			}
		}
	}*/

	return true;
}

bool Event_MouseMoved(void* pEventListener, EventData_t data)
{
	f32 mouseXPos = data.f32[0];
	f32 mouseYPos = data.f32[1];

	//LOG_DEBUG("Mouse Pos: X:%f Y:%f", mouseXPos, mouseYPos);
	return true;
}

static bool Event_WindowResized(void* pEventListener, EventData_t data)
{
	u32 width, height;
	width = data.u32[0];
	height = data.u32[1];

	LOG_DEBUG("Window Resized: %d / %d", width, height);

	if (width == 0 || height == 0)
	{
		s_bApplicationSuspended = true;
	}
	else
	{
		s_bApplicationSuspended = false;
	}

	if (!s_bApplicationSuspended)
	{
		// Let the app handle any necessary logic.
		s_Engine.GetApplication()->OnWindowResized(width, height);
	}

	return false;
}

static bool Event_WindowResizedPause(void* pEventListener, EventData_t data)
{
	s_bApplicationSuspended = true;
	return true;
}

static bool Event_WindowResizedUnpause(void* pEventListener, EventData_t data)
{
	s_bApplicationSuspended = false;
	return true;
}

static void ConsoleCommand_LoadMap(struct ConVar_t* pCommand, void* pValue)
{
	if (!pValue)
	{
		LOG_WARN("Console command 'map' requires a map name to load.");
		return;
	}

	LOG_DEBUG("Loading map: '%s'.", (char*)pValue);

	Clock mapLoadTimer{};
	mapLoadTimer.Start();

	IMaterialSystem* pMaterialSystem = g_pEngine->GetEngineInterfaces()->pMaterialSystem;
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	// HACK HACK: Just need the map to render.
	EngineInterfaces_t* pInterfaces = const_cast<EngineInterfaces_t*>(g_pEngine->GetEngineInterfaces());

	if (!LoadMap((char*)pValue, pMeshSystem, pMaterialSystem, *pInterfaces->pCurrentScene))
	{
		return;
	}

	LOG_DEBUG("Memory usage post-map resource free: %.10f", g_pMemoryManager->GetPercentAllocated());
	LOG_WARN("Map load took %.5f", mapLoadTimer.GetElapsedTime());
}

//-------------------------------------------------------------------------
// Purpose: Registers our .dll interfaces with the core registry.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void ReturnInterfacesToRegistry()
{
	LOG_DEBUG("Engine.dll passing interfaces down to Core.dll");
	LOG_WARN("REMOVE THIS FUNCTION!");
}

//-------------------------------------------------------------------------
// An callback that is exported so other libraries can call window resize events.
//-------------------------------------------------------------------------
bool WindowResizeHandler(u32 width, u32 height)
{
	EventData_t data{};
	data.u32[0] = width;
	data.u32[1] = height;
	s_Engine.GetEngineInterfaces()->pEngineEventHandler->EventFire(eEventCategory::WINDOW_RESIZE, nullptr, data);

	return true;
}

//-------------------------------------------------------------------------
// An callback that allows the window to fire engine-shutdown events.
//-------------------------------------------------------------------------
void WindowClosedHandler()
{
	// Fire an event to close the application.
	s_Engine.GetEngineInterfaces()->pEngineEventHandler->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
}
