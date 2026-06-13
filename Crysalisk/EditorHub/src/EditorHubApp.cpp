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

	// Init camera data.
	m_Camera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_Camera.RecalculateCameraVectors();

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Run()
{
	// Serve our tools.
	// EDITORTODO: Only run the active tool!
	for (i32 iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Run();
	}

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
