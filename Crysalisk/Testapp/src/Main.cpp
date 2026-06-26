//=============================================================================
// Author: Jared Simons
// Date Created: 14/2/2024
// 
// Purpose: The game executable's entrypoint.
//=============================================================================
#include <Application.h>

#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Logging.h>
#include <Renderer/Camera.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Resources/Map.h>
#include <Systems/MaterialSystem.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>
#include <Renderer/Views/UI_View.h>

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
	virtual bool	Run() override;
	virtual bool	Render() override;
	virtual void	Shutdown() override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;

	virtual Camera_t* GetMainCamera() override { return &m_MainCamera; }

private:
	ApplicationConfig_t*	m_pApplicationConfig{ nullptr };
	Camera_t				m_MainCamera{};
	
	// These need access to our members.
	friend static bool Event_WindowResized(void* pEventListener, EventData_t data);
};

static CGameApp s_GameApp{};

static bool Event_WindowResized(void* pEventListener, EventData_t data);

i32 main(i32 argc, char** argv)
{
	ApplicationConfig_t appConfig{};
	appConfig.pApplication = &s_GameApp;

	return Crysalisk_Main(appConfig);
}

bool CGameApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());
	pAppConfig->pViews.PushBack(RenderViewCreate_UIView());

	return true;
}

bool CGameApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_DEBUG("Initializing Testapp");

	// Start listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::WINDOW_RESIZE, this, Event_WindowResized);

	// Init camera data.
	m_MainCamera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_MainCamera.RecalculateCameraVectors();

	return true;
}

void CGameApp::Shutdown()
{
	// Stop listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventUnregister(eEventCategory::WINDOW_RESIZE, this, Event_WindowResized);

	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();
	m_pApplicationConfig->pViews.Clear();
}

bool CGameApp::Run()
{
	return true;
}

void CGameApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
	m_MainCamera.UpdateCameraData(m_pApplicationConfig->pEngine->GetEngineInterfaces()->pInputSystem, pRenderPacket->DeltaTime);
	pRenderPacket->pCamera = &m_MainCamera;
}

bool CGameApp::Render()
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
