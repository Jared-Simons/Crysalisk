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

#include "ToolsTypes.h"

class CEditorHubApp : public ICrysApplication
{
public:
	virtual bool Application_Init() override;
	virtual bool Application_Run() override;
	virtual void Application_Shutdown() override;

private:
	virtual bool LoadTool(const char* pszToolLibraryName);

private:
	Utl::CVector<IToolApp*> m_pTools{};
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

bool CEditorHubApp::Application_Init()
{
	LOG_INFO("Launching Crysalisk Tools");
	m_pTools.Resize(1);

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Application_Run()
{
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
