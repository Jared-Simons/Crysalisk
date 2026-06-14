//=============================================================================
// Author: Jared Simons
// Date Created: 12/12/2025
// 
// Purpose: The entry point for the main editor executable.
//=============================================================================
#include <Application.h>
#include <Core/Assertions.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Strings/String_Utils.h>

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) i32 Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

typedef bool (*PFN_Tool_Initialize)();
typedef bool (*PFN_Tool_Run)();
typedef ICrysApplication* (*PFN_Get_Application_State)();

static Platform::ISysModule* pToolModule = nullptr;
PFN_Tool_Run pfnRun = nullptr;

bool LoadTool(const char* pszToolName);

i32 main(i32 argc, char** argv)
{
	// Load up the editor library.
	// FIXME: There's a string leak happening here on shutdown.
	Platform::ISysModule* pEditorHubDLL = Platform::LoadSysModule("EditorHub");
	PFN_Get_Application_State pfnGetAppState = (PFN_Get_Application_State)pEditorHubDLL->GetFuncAddr("GetApplicationState");
	CAssert(pfnGetAppState);

	// We need to launch the engine as well.
	ApplicationConfig_t appConfig{};

	appConfig.pApplication = pfnGetAppState();

	// Start up the engine.
	Crysalisk_Main(appConfig);

	// EDITORTODO: Launch the tool specified by some argument (command-line? tool-selector menu?)

	return 0;
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
