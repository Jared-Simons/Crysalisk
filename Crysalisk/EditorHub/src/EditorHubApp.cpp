//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: EDITORTODO: Documentation.
//=============================================================================
#include <Application.h>
#include <Containers/CVector.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Renderer/Camera.h>
#include <Renderer/Renderer.h>
#include <Strings/String_Utils.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>
#include <Renderer/Views/UI_View.h>

// Framework includes.
#include <AppFramework.h>

#include "ToolsTypes.h"

class CEditorHubApp : public ICrysApplication
{
public:
	virtual bool PreInitialize(ApplicationConfig_t* pAppConfig);
	virtual bool Initialize(ApplicationConfig_t* pAppConfig) override;
	virtual bool Run() override;
	virtual void FramePrepare(struct RenderPacket_t* pRenderPacket) override;
	virtual bool Render() override;
	virtual void Shutdown() override;	

	// NOTE: No camera here for now.
	virtual Camera_t* GetMainCamera() override { return &m_Camera; }

	virtual bool LoadTool(const char* pszToolLibraryName);

private:
	ApplicationConfig_t*			m_pApplicationConfig{ nullptr };
	AppFramework::IAppFramework*	m_pAppFramework{ nullptr };
	Utl::CVector<IToolApp*>			m_pTools{};
	Camera_t						m_Camera{};
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

bool CEditorHubApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());
	pAppConfig->pViews.PushBack(RenderViewCreate_UIView());

	return true;
}

typedef void* (*PFN_GetAppFrameworkState)();

bool CEditorHubApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Launching Crysalisk Tools");
	m_pTools.Resize(1);

	// Load the App Framework module.
	Platform::ISysModule* pUIModule = Platform::LoadSysModule("AppFramework");
	if (!pUIModule)
	{
		LOG_FATAL("Failed to load AppFramework.dll! Application cannot continue.");
		return false;
	}

	PFN_GetAppFrameworkState pfnGetAppFramework = (PFN_GetAppFrameworkState)pUIModule->GetFuncAddr("GetAppFrameworkState");
	if (!pfnGetAppFramework)
	{
		LOG_ERROR("Failed to get a pointer to the app framework state. Initialization failed.");
		return false;
	}
	m_pAppFramework = (AppFramework::IAppFramework*)pfnGetAppFramework();
	CAssert(m_pAppFramework != nullptr);

	// Init camera data.
	m_Camera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_Camera.RecalculateCameraVectors();

	// Initialize the framework.
	AppFramework::AppFrameworkConfig_t appConfig{};
	appConfig.pEngine = pAppConfig->pEngine;
	/*if (!m_pAppFramework->Initialize(appConfig))
	{
		LOG_ERROR("Failed to initialize app framework! Application cannot continue.");
		return false;
	}*/

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Run()
{
	// Serve our tools.
	// EDITORTODO: Only run the active tool!
	for (int iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Run();
	}

	m_pAppFramework->Run();

	//RenderPacket_t packet{};
	//m_pApplicationConfig->pEngine->GetEngineInterfaces()->pRenderer->Update(m_pAppFramework->GetActiveWindow(), packet, nullptr);

	return true;
}

void CEditorHubApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
	pRenderPacket->pCamera = &m_Camera;
}

bool CEditorHubApp::Render()
{
	return true;
}

void CEditorHubApp::Shutdown()
{
	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();
	m_pTools.Clear();

	//m_pAppFramework->Shutdown();
}

bool CEditorHubApp::LoadTool(const char* pszToolLibraryName)
{
	CAssert(pszToolLibraryName);

	u64 iNameLength = Utl::StringLength(pszToolLibraryName);
	CAssert(iNameLength > 0 && iNameLength < 10000);
	
	Platform::ISysModule* pToolModule = Platform::LoadSysModule(pszToolLibraryName);
	PFN_Get_Tool_App_State pfnGetToolAppState = (PFN_Get_Tool_App_State)pToolModule->GetFuncAddr("GetToolApplicationState");
	CAssert(pfnGetToolAppState);

	m_pTools.PushBack(pfnGetToolAppState());
	m_pTools.Back()->pToolModule = pToolModule;

	// Initialize the new tool.
	m_pTools.Back()->Tool_Init();

	return true;
}

ICrysApplication* GetApplicationState()
{
	return &s_EditorHubApp;
}
