//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2024
// 
// Purpose: The entrypoint to the level-editor.
//=============================================================================
#include <stdio.h> // EDITORTODO: Remove me!

#include <ToolsTypes.h>

class CLevelEditorApp : public IToolApp
{
public:
	virtual bool	Tool_Init() override;
	virtual bool	Tool_Run() override;
	virtual void	Tool_Shutdown() override;
};

static CLevelEditorApp s_LevelEditorApp{};

extern "C"
{
	__declspec(dllexport) IToolApp* GetToolApplicationState();
}

bool CLevelEditorApp::Tool_Init()
{
	printf("Welcome to Crysalisk Level Editor!\n");

	return true;
}

bool CLevelEditorApp::Tool_Run()
{
	printf("Running Level Editor Loop!\n");
	return true;
}

void CLevelEditorApp::Tool_Shutdown()
{

}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}
