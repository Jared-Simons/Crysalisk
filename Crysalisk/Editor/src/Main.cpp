//=============================================================================
// Author: Jared Simons
// Date Created: 12/12/2025
// 
// Purpose: The entry point for the main editor executable.
//=============================================================================
#include <EntryPoint.h>

#include <Application.h>
#include <Core/Assertions.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Strings/String_Utils.h>

typedef bool (*PFN_Tool_Initialize)();
typedef bool (*PFN_Tool_Run)();
typedef ICrysApplication* (*PFN_Get_Application_State)();

static Platform::ISysModule* pToolModule = nullptr;
PFN_Tool_Run pfnRun = nullptr;

bool LoadTool(const char* pszToolName);

bool CreateApplication(ApplicationConfig_t* pAppConfig)
{
	// Load up the editor library.
	Platform::ISysModule* pEditorHubDLL = Platform::LoadSysModule("EditorHub");
	PFN_Get_Application_State pfnGetAppState = (PFN_Get_Application_State)pEditorHubDLL->GetFuncAddr("GetApplicationState");
	CAssert(pfnGetAppState);
	pAppConfig->pApplication = pfnGetAppState();

	return true;
}

bool LoadTool(const char* pszToolName)
{
	CAssert(pszToolName);

	u64 strLen = Utl::StringLength(pszToolName);
	char szToolLibName[10000]{};
	CAssert(strLen < 10000);
	Platform::MemoryCopy(szToolLibName, pszToolName, strLen);

	pToolModule = Platform::LoadSysModule(pszToolName);

	if (pToolModule == nullptr)
	{
		// EDITORTODO: Logging.
		return false;
	}

	// Load up PFNs to initialize the tools.
	PFN_Tool_Initialize pfnInit = (PFN_Tool_Initialize)pToolModule->GetFuncAddr("Tools_Init");
	if (!pfnInit)
	{
		// EDITORTODO: Logging.
		return false;
	}

	return pfnInit();
}
