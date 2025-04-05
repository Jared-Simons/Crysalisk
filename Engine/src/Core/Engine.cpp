//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Implements the core engine loop.
//=============================================================================

#include "Engine.h"

#include <Containers/CHashMap.h>
#include <Containers/CVector.h>
#include <Core/Event.h>
#include <Core/Input.h>
#include <Core/Logging.h>
#include <Interfaces.h>
#include <Memory/MemoryManager.h>
#include <Platform/InterfaceRegistry.h>
#include <Platform/Platform.h>
#include "Renderer/Renderer.h"
#include "Renderer/RendererTypes.h"
#include "Resources/GeometryGenerator.h"
#include <Strings/String_Utils.h>
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"

static bool s_bApplicationQuitCalled{ false };
static bool s_bApplicationSuspended{ false };

// Engine Event Handlers
static bool Event_ApplicationQuit(EventData_t data);
static bool Event_KeyPressed(EventData_t data);
static bool Event_KeyReleased(EventData_t data);
static bool Event_MouseButtonPressed(EventData_t data);
static bool Event_MouseButtonReleased(EventData_t data);
static bool Event_MouseMoved(EventData_t data);
static bool Event_WindowResized(EventData_t data);
static bool Event_WindowResizedPause(EventData_t data);
static bool Event_WindowResizedUnpause(EventData_t data);

//-------------------------------------------------------------------------
// Implementation of the core engine class
//-------------------------------------------------------------------------
class CEngine : public IEngine
{
public:
	virtual bool	Initialize(EngineConfig_t engineConfig) override;
	virtual bool	Run() override;
	virtual void	Shutdown() override;

private:
	EngineConfig_t	m_engineConfig;

	Platform::IPlatformWindow* m_pEngineWindow = nullptr;
};

// Engine context.
static CEngine s_Engine;
IEngine* g_pEngine = &s_Engine;

//-------------------------------------------------------------------------
// Purpose: Initialize the engine and it's required subsystems.
// Input: engineConfig - A configuration passed in by the calling application.
// Output: bool - Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool CEngine::Initialize(EngineConfig_t engineConfig)
{
	m_engineConfig = engineConfig;

	// Stand up our memory manager.
	ApplicationMemoryConfiguration_t memoryConfig{};
	memoryConfig.totalMemoryBudget = MEBIBYTES(1);
	if (!IMemoryManager::CreateMemorySystem(memoryConfig))
	{
		// This error is handled by a platform-specific message box, as the logging system technically isn't set up yet.
		Platform::DisplayMessageBox(nullptr, "Memory manager failed to initialize! Application cannot continue.", "Engine Error", false);
		return false;
	}

	// Logger.
	if (!g_pLog->Init((int)eLogLevel::LOG_LEVEL_VERBOSE)) // Log all messages
	{
		// This error is handled by a platform-specific message box, as the logging system technically failed to initialize.
		Platform::DisplayMessageBox(nullptr, "Logger failed to initialize! Application cannot continue.", "Engine Error", false);
		return false;
	}

	// Event System.
	if (!g_pEngineEvents_Internal->Initialize())
	{
		LOG_FATAL("Event system failed to initialize! Application cannot continue.");
		return false;
	}

	if (!Platform::InitializePlatformLayer())
	{
		LOG_FATAL("Failed to initialize platform layer! Application cannot continue.");
		return false;
	}

	Platform::WindowParams_t windowParams{};
	windowParams.Name = "Crysalisk Engine\0";
	windowParams.X = 100;
	windowParams.Y = 100;
	windowParams.Width = 800;
	windowParams.Height = 600;
	m_pEngineWindow = Platform::CreatePlatformWindow(windowParams, true);
	m_pEngineWindow->SetWindowResizeCallback(WindowResizeHandler);

	// Load up external libraries now, so they can get interfaces prior to initialization.
	Platform::ISysModule* pRendererModule = Platform::LoadSysModule("Renderer_Vulkan.dll");
	if (!pRendererModule)
	{
		LOG_FATAL("Failed to load Renderer_Vulkan.dll! Application cannot continue.");
		return false;
	}

	// Get everyone's interfaces!
	Platform::ObtainLibraryInterfaces();

	// Make sure everyone gets their interfaces!
	ReturnInterfacesToRegistry();
	AssignInterfaces(Platform::GetRegisteredInterfaces());
	Platform::ConnectLibraryInterfaces();

	// The Core.dll gets it's interfaces separately because it is linked at link-time (TODO: Maybe this should work like every other lib?)
	Platform::SetupExternalInterfaces();

	// Input System.
	if (!g_pInputSystem_Internal->Initialize(*m_pEngineWindow))
	{
		LOG_FATAL("Input system failed to initialize! Application cannot continue.");
		return false;
	}

	// Register for engine-level events.
	g_pEngineEvents->EventRegister(eEventCategory::APPLICATION_QUIT, nullptr, Event_ApplicationQuit);
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, Event_KeyPressed);
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_KEY_RELEASED, nullptr, Event_KeyReleased);
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, nullptr, Event_MouseButtonPressed);
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, nullptr, Event_MouseButtonReleased);
	g_pEngineEvents->EventRegister(eEventCategory::INPUT_MOUSE_MOVED, nullptr, Event_MouseMoved);

	// Renderer
	RendererConfig_t rendererConfig{};
	rendererConfig.szApplicationName = "Crysalisk Engine"; // TODO: Pass this in from an app config.
	rendererConfig.pHWnd = m_pEngineWindow->GetInternalHandle();
	rendererConfig.pRendererBackendDLL = pRendererModule;
	rendererConfig.MaxTextures = 4999;
	rendererConfig.MaxPipelines = 300;

	if (!g_pRenderer->Initialize(rendererConfig))
	{
		LOG_FATAL("Renderer failed to initialize! Application cannot continue.");
		return false;
	}

	// Texture manager.
	TextureSystemConfig_t textureSysConfig{};
	textureSysConfig.MaxTextures = 4999;
	CAssert(rendererConfig.MaxTextures == textureSysConfig.MaxTextures);
	if (!g_pTextureSystem->Initialize(textureSysConfig))
	{
		LOG_ERROR("Failed to initialize texture system! Application cannot continue.");
		return false;
	}

	// Shader system.
	ShaderSystemConfig_t shaderSysConfig{};
	shaderSysConfig.MaxShaders = 197;
	if (!g_pShaderSystem->Initialize(shaderSysConfig))
	{
		LOG_ERROR("Failed to initialize shader system! Application cannot continue.");
		return false;
	}

	// Material system.
	MaterialSystemConfig_t materialSysConfig{};
	materialSysConfig.MaxMaterials = 2049;
	if (!g_pMaterialSystemInternal->Initialize(materialSysConfig))
	{
		LOG_ERROR("Failed to initialize material system! Application cannot continue.");
		return false;
	}

	MaterialHandle_t testMaterialHandle = g_pMaterialSystemInternal->AcquireMaterial("Assets/Materials/testmaterial");
	//ShaderHandle_t testShaderHandle = g_pShaderSystem->AcquireShader("Assets/Shaders/CoreShader.cshadercfg");

	// Create temporary geometry.
	g_pRenderer->CreateTempGeometry();

	LOG_INFO("Engine Finished Initialization");

	// Print memory state
	g_pMemoryManager->PrintMemoryStats();
	
	return true;
}

//-------------------------------------------------------------------------
// Runs a single engine frame.
//-------------------------------------------------------------------------
bool CEngine::Run()
{
	while (!s_bApplicationQuitCalled)
	{
		Platform::ProcessMessages();
		// NOTE: We need to check for the application quit event again here, because it might've been set during 'Platform::ProcessMessages()' 
		// and trying to run another frame may result in a fatal crash (the renderer attempting to render to a window that no longer exists, for example).
		if (!s_bApplicationSuspended && !s_bApplicationQuitCalled) 
		{
			g_pInputSystem_Internal->Update();

			RenderPacket_t frameRenderData{};

			g_pRenderer->Update(frameRenderData);
		}
	}

	Shutdown();
	return true;
}

//-------------------------------------------------------------------------
// Shutdows down the engine.
//-------------------------------------------------------------------------
void CEngine::Shutdown()
{
	LOG_INFO("EngineShutdown() called!");

	g_pMaterialSystemInternal->Shutdown();
	g_pShaderSystem->Shutdown();
	g_pTextureSystem->Shutdown();

	g_pRenderer->Shutdown();

	// Release our main window.
	if (m_pEngineWindow)
	{
		delete m_pEngineWindow;
		m_pEngineWindow = nullptr;
	}

	Platform::ShutdownPlatformLayer();
	g_pInputSystem->Shutdown();
	g_pEngineEvents->Shutdown();
	g_pLog->Shutdown();
	g_pMemoryManager->Shutdown();
}

//-------------------------------------------------------------------------
// Engine-level event handlers.
//-------------------------------------------------------------------------
bool Event_ApplicationQuit(EventData_t data)
{
	LOG_VERBOSE("Application_Quit() called");
	s_bApplicationQuitCalled = true;
	return true;
}

bool Event_KeyPressed(EventData_t data)
{
	char keycode = *data.c;
	//if (keycode >= 65 && keycode <= 90)
		//LOG_DEBUG("%c Key Pressed", keycode);
	return true;
}

bool Event_KeyReleased(EventData_t data)
{
	char keycode = *data.c;
	//if (keycode >= 65 && keycode <= 90)
		//LOG_DEBUG("%c Key Released", keycode);

	if (keycode == KEY_ESCAPE)
	{
		// Time to shutdown the application!
		g_pEngineEvents->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
	}

	if (keycode == 'M')
	{
		// TODO: Print Memory Stats
	}

	return true;
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

bool Event_MouseMoved(EventData_t data)
{
	float mouseXPos = data.f32[0];
	float mouseYPos = data.f32[1];

	//LOG_DEBUG("Mouse Pos: X:%f Y:%f", mouseXPos, mouseYPos);
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
		s_bApplicationSuspended = true;
	}
	else
	{
		s_bApplicationSuspended = false;
	}

	return true;
}

static bool Event_WindowResizedPause(EventData_t data)
{
	s_bApplicationSuspended = true;
	return true;
}

static bool Event_WindowResizedUnpause(EventData_t data)
{
	s_bApplicationSuspended = false;
	return true;
}

//-------------------------------------------------------------------------
// Method to pass our interface objects down to the interface registry.
//-------------------------------------------------------------------------
bool ReturnInterfacesToRegistry()
{
	LOG_DEBUG("Engine.dll passing interfaces down to Core.dll");

	// This does nothing in the engine.dll
	// TODO: How should the engine implicitly pass interfaces back to the core registry, just like any other dll?

	Platform::SystemInterface_t eventSystem{};
	eventSystem.pSystemAddr = reinterpret_cast<void*>(g_pEngineEvents_Internal);
	eventSystem.szSystemName = StringDuplicate("engine_events");
	Platform::RegisterInterface(eventSystem);

	Platform::SystemInterface_t inputSystem{};
	inputSystem.pSystemAddr = reinterpret_cast<void*>(g_pInputSystem_Internal);
	inputSystem.szSystemName = StringDuplicate("input_system");
	Platform::RegisterInterface(inputSystem);

	Platform::SystemInterface_t materialSystem{};
	materialSystem.pSystemAddr = reinterpret_cast<void*>(g_pMaterialSystemInternal);
	materialSystem.szSystemName = StringDuplicate("material_system");
	Platform::RegisterInterface(materialSystem);

	return true;
}

//-------------------------------------------------------------------------
// An callback that is exported so other libraries can call window resize events.
//-------------------------------------------------------------------------
bool WindowResizeHandler(unsigned int width, unsigned int height)
{
	EventData_t data{};
	data.u32[0] = width;
	data.u32[1] = height;
	g_pEngineEvents->EventFire(eEventCategory::WINDOW_RESIZE, nullptr, data);

	return true;
}