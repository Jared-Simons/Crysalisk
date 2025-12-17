#include <stdio.h>

#include <Core/Assertions.h>
#include <Platform/Platform.h>
#include <Strings/String_Utils.h>

typedef bool (*PFN_Tool_Initialize)();
typedef bool (*PFN_Tool_Run)();

static Platform::ISysModule* pToolModule = nullptr;
PFN_Tool_Run pfnRun = nullptr;

bool LoadTool(const char* pszToolName);

int main(int argc, char** argv)
{
	printf("Launching Crysalisk Tools\n");

	// Launch the tool specified by the command-line arguments.
	if (argc < 2)
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
		
	}

	return 0;
}

bool LoadTool(const char* pszToolName)
{
	CAssert(pszToolName);

	size_t strLen = Utl::StringLength(pszToolName);
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
