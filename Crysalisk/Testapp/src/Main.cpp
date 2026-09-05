//=============================================================================
// Author: Jared Simons
// Date Created: 14/2/2024
// 
// Purpose: The game executable's entrypoint.
//=============================================================================
#include <EntryPoint.h>
#include <Application.h>

#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Logging.h>
#include <Renderer/Camera.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>

// UI Framework.
#include <AppUI.h>

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) i32 Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

class CGameApp : public ICrysApplication
{
public:
	virtual bool	PreInitialize(ApplicationConfig_t* pAppConfig) override;
	virtual bool	Initialize(ApplicationConfig_t* pAppConfig) override;
	virtual void	Shutdown() override;

	virtual bool	Run(f32 deltaTime) override;
	virtual bool	Render(struct RenderPacket_t* pRenderPacket) override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;

	virtual void	OnWindowResized(u16 width, u16 height) override {}	// NOTE: Does nothing for now.

	virtual Camera_t* GetMainCamera() override { return &m_MainCamera; }

private:
	ApplicationConfig_t*	m_pApplicationConfig{ nullptr };
	Camera_t				m_MainCamera{};

	AppUI::IAppUI*				m_pUISystem{ nullptr };
	AppUI::UIControlHandle_t	m_hProfilingText{ kInvalidID_U32 };
	
	// These need access to our members.
	friend static bool Event_WindowResized(void* pEventListener, EventData_t data);
};

static CGameApp s_GameApp{};

static bool Event_WindowResized(void* pEventListener, EventData_t data);

bool CGameApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());

	// Load the UI library.
	Platform::ISysModule* pAppUILibrary = Platform::LoadSysModule("AppUI");
	PFN_GetAppFrameworkState pfnGetPluginInterface = (PFN_GetAppFrameworkState)pAppUILibrary->GetFuncAddr("GetUIPluginInterface");
	CAssert(pfnGetPluginInterface);

	m_pUISystem = pfnGetPluginInterface();
	m_pUISystem->PreInitialize(pAppConfig);

	return true;
}

bool CGameApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_DEBUG("Initializing Testapp");

	m_pUISystem->Initialize(pAppConfig);

	// Setup profiling text control.
	m_hProfilingText = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	m_pUISystem->SetControlPosition(m_hProfilingText, 25, 575);
	m_pUISystem->SetControlFont(m_hProfilingText, "UbuntuMono-R.ttf", 30.0f);

	// Start listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::WINDOW_RESIZE, this, Event_WindowResized);

	// Init camera data.
	m_MainCamera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_MainCamera.RecalculateCameraVectors();

	// Can set the clear color from here.
	IRenderView* pSceneView = g_pEngine->GetEngineInterfaces()->pRenderer->GetRenderView("3D Scene View");
	((Renderpass*)pSceneView->GetRenderpass())->clearColor = { 0.3f, 0.0f, 0.3f };

	return true;
}

void CGameApp::Shutdown()
{
	// Shutdown the UI system.
	m_pUISystem->Shutdown();

	// Stop listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventUnregister(eEventCategory::WINDOW_RESIZE, this, Event_WindowResized);

	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();
	m_pApplicationConfig->pViews.Clear();
}

bool CGameApp::Run(f32 deltaTime)
{
	return true;
}

void CGameApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
	// Profiling data.
	// FIXME: Make sure the metrics data is always available.
	// FIXME: The engine can't link with AppUI because AppUI depends on engine. But I would like this text (and all UI tools) to be able to be shared between engine and tools. Investigate.
	if (pRenderPacket->pMetrics)
	{
		RenderMetrics_t& pMetrics = *pRenderPacket->pMetrics;
		char szMetricsString[512]{};
		Utl::StringFormat_V("Frametime: %.5f s (FPS: %d)\nDraw calls: %d (Geometry culled: %d)\nCurrent memory pool usage: %.5f\nPosition: %.3f, %.3f, %.3f\nDebug vis mode: %s",
			szMetricsString,
			pMetrics.FrameTime, pMetrics.PreviousFrameFPS,
			pMetrics.NumDrawCalls, pRenderPacket->pMetrics->NumGeometriesFrustumCulled,
			g_pMemoryManager->GetPercentAllocated(),
			m_MainCamera.vecPosition.x,
			m_MainCamera.vecPosition.y,
			m_MainCamera.vecPosition.z,
			m_pApplicationConfig->pEngine->GetEngineInterfaces()->pRenderer->GetRendererDebugVisModeName());
		m_pUISystem->SetControlText(m_hProfilingText, szMetricsString);
	}

	m_MainCamera.UpdateCameraData(m_pApplicationConfig->pEngine->GetEngineInterfaces()->pInputSystem, pRenderPacket->DeltaTime);
	pRenderPacket->pCamera = &m_MainCamera;

	m_pUISystem->FramePrepare(pRenderPacket);
}

bool CGameApp::Render(struct RenderPacket_t* pRenderPacket)
{
	IRenderer* pRenderer = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pRenderer;
	CAssert(pRenderer);
	
	return true;
}

static bool Event_WindowResized(void* pEventListener, EventData_t data)
{
	LOG_DEBUG("APP WINDOW RESIZED!");
	CGameApp* pGameState = (CGameApp*)pEventListener;

	// Update camera state.
	if (data.u32[0] > 0.0f)	// not minimized
	{
		pGameState->m_MainCamera.bAspectRatioChanged = true;
	}

	// Allow other handlers to get this event.
	return false;
}

bool CreateApplication(ApplicationConfig_t* pAppConfig)
{
	// NOTE: Just a stub for now.
	pAppConfig->pApplication = &s_GameApp;

	return true;
}
