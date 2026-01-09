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
#include <Strings/String_Utils.h>

// Framework includes.
#include <UIContext.h>

#include "ToolsTypes.h"

class CEditorHubApp : public ICrysApplication
{
public:
	virtual bool Application_Init(ApplicationConfig_t* pAppConfig) override;
	virtual bool Application_Run() override;
	virtual void Application_Shutdown() override;
	
	virtual void FramePrepare(struct RenderPacket_t* pRenderPacket) override {};

private:
	virtual bool LoadTool(const char* pszToolLibraryName);

private:
	Utl::CVector<IToolApp*> m_pTools{};
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

bool CEditorHubApp::Application_Init(ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Launching Crysalisk Tools");
	m_pTools.Resize(1);

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	Platform::WindowParams_t windowParams{};
	windowParams.pszName = "Crysalisk Tools";
	windowParams.X = 100;
	windowParams.Y = 100;
	windowParams.Width = 1080;
	windowParams.Height = 720;

	AppFramework::UIAppConfig_t appConfig{};
	appConfig.pWindow = windowParams;
	appConfig.pRendererLibrary = pAppConfig->pRendererLibrary;

	AppFramework::CreateUIContext(appConfig);

	return true;
}

bool CEditorHubApp::Application_Run()
{
	if (!AppFramework::ShouldRunUIContext())
	{
		// TODO: This is just a quick, easy way to quit the engine.
		return false;
	}

	AppFramework::RunUIContext();

	// Serve our tools.
	// EDITORTODO: Only run the active tool!
	for (int iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Run();
	}

	return true;
}

void CEditorHubApp::Application_Shutdown()
{
	AppFramework::ShutdownUIContext();

	m_pTools.Clear();
}

bool CEditorHubApp::LoadTool(const char* pszToolLibraryName)
{
	CAssert(pszToolLibraryName);

	size_t iNameLength = Utl::StringLength(pszToolLibraryName);
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
