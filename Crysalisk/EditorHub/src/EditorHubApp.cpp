//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: EDITORTODO: Documentation.
//=============================================================================
#include <Application.h>
#include <Containers/CVector.h>
#include <Core/Engine.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Renderer/Camera.h>
#include <Renderer/Renderer.h>
#include <Strings/String_Utils.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>
#include <Renderer/Views/UI_View.h>

// UI Framework
#include <AppUI.h>

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
	Utl::CVector<IToolApp*>			m_pTools{};
	Camera_t						m_Camera{};

	AppUI::IAppUI*					m_pUISystem{ nullptr };
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();
typedef AppUI::IAppUI* (*PFN_GetAppFrameworkState)();

bool CEditorHubApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());

	// Load up UI library.
	// FIXME: Putting this here so that the UI Plugin's render view gets added before the UI view below. There's gotta be a better way to do this.
	Platform::ISysModule* pAppUILibrary = Platform::LoadSysModule("AppUI");
	PFN_GetAppFrameworkState pfnGetPluginInterface = (PFN_GetAppFrameworkState)pAppUILibrary->GetFuncAddr("GetUIPluginInterface");
	CAssert(pfnGetPluginInterface);

	m_pUISystem = pfnGetPluginInterface();
	CAssert(m_pUISystem);
	m_pUISystem->PreInitialize(pAppConfig);

	pAppConfig->pViews.PushBack(RenderViewCreate_UIView());

	return true;
}

// HACK: UI test
static AppUI::UIControlHandle_t hTestFrame = kInvalidID_U32;
static AppUI::UIControlHandle_t	hTestLabel = kInvalidID_U32;

bool CEditorHubApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Launching Crysalisk Tools");
	m_pTools.Resize(1);

	m_pUISystem->Initialize(pAppConfig);

	// Init camera data.
	m_Camera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_Camera.RecalculateCameraVectors();

	// HACK: UI test
	{
		// NOTE: Base resolution that UI elements are defined in is 1080x720.
		
		//hTestFrame = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_FRAME);
		hTestFrame = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUISystem->SetControlPosition(hTestFrame, 50, 50);
		m_pUISystem->SetControlDimensions(hTestFrame, 700, 525);
		m_pUISystem->SetControlColor(hTestFrame, 255, 255, 255, 200);

		hTestLabel = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUISystem->SetControlText(hTestLabel, "TEST LABEL TEXT");

		//hTestButton = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
		//m_pUISystem->SetControlPosition(hTestButton, 400, 0);
		//m_pUISystem->SetControlDimensions(hTestButton, 50, 650);
		//m_pUISystem->SetControlColor(hTestButton, 255, 0, 0, 255);
	}

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Run()
{
	// Update the UI plugin.
	m_pUISystem->Update();

	// Serve our tools.
	// EDITORTODO: Only run the active tool!
	for (i32 iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Run();
	}

	return true;
}

void CEditorHubApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
	// LEFTOFF: We need a way to populate UI-specific geometry data in the render packet from this point.
	// We can't do the 'extended data' that we did with other geometries by simply indexing the void* array
	// because this view will not exist in every build.
	m_pUISystem->FramePrepare(pRenderPacket);

	m_Camera.UpdateCameraData(m_pApplicationConfig->pEngine->GetEngineInterfaces()->pInputSystem, pRenderPacket->DeltaTime);
	pRenderPacket->pCamera = &m_Camera;
}

bool CEditorHubApp::Render()
{
	return true;
}

void CEditorHubApp::Shutdown()
{
	//m_pUISystem->DestroyUIControl(hTestButton);
	//m_pUISystem->DestroyUIControl(hTestPanel);
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
