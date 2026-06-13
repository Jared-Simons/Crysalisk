//=============================================================================
// Author: Jared Simons
// Date Created: 12/12/2025
// 
// Purpose: The entry point for the main editor executable.
//=============================================================================
#include <stdio.h> // EDITORTODO: Remove Me!

#include <Application.h>
#include <Core/Assertions.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Strings/String_Utils.h>

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) int Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

typedef bool (*PFN_Tool_Initialize)();
typedef bool (*PFN_Tool_Run)();
typedef ICrysApplication* (*PFN_Get_Application_State)();

static Platform::ISysModule* pToolModule = nullptr;
PFN_Tool_Run pfnRun = nullptr;

bool LoadTool(const char* pszToolName);

int main(int argc, char** argv)
{
	// Load up the editor dll.
	Platform::ISysModule* pEditorHubDLL = Platform::LoadSysModule("EditorHub");
	PFN_Get_Application_State pfnGetAppState = (PFN_Get_Application_State)pEditorHubDLL->GetFuncAddr("GetApplicationState");
	CAssert(pfnGetAppState);

	// We need to launch the engine as well.
	ApplicationConfig_t appConfig{};

	appConfig.pApplication = pfnGetAppState();

	Crysalisk_Main(appConfig);

	// Launch the tool specified by the command-line arguments.
	/*if (argc < 2)
	{
		printf("Error! Crysalisk tools requires an argument for the tool to launch!\n");
		return -1;
	}

	if (Utl::StringCompareInsensitive(argv[1], "-map_editor"))
	{
		printf("Launching Crysalisk Map Editor!\n");
		LoadTool("Tools_LevelEditor");
	}
	else
	{
		printf("Error: Unrecognized tool parameter: '%s'.\n", argv[1]);
		return -2;
	}

	pfnRun = (PFN_Tool_Run)pToolModule->GetFuncAddr("Tools_Run");
	CAssert(pfnRun);

	while (pfnRun())
	{
		
	}*/

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
