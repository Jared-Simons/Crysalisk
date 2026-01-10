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
	virtual bool PreInitialize(ApplicationConfig_t* pAppConfig);
	virtual bool Initialize(ApplicationConfig_t* pAppConfig) override;
	virtual bool Run() override;
	virtual void Shutdown() override;
	
	virtual void FramePrepare(struct RenderPacket_t* pRenderPacket) override {};

private:
	void			ConfigureRenderViews(ApplicationConfig_t* pAppConfig);

	virtual bool LoadTool(const char* pszToolLibraryName);

private:
	ApplicationConfig_t*	m_pApplicationConfig{ nullptr };
	Utl::CVector<IToolApp*> m_pTools{};
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

bool CEditorHubApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup the render views.
	ConfigureRenderViews(pAppConfig);

	return true;
}

bool CEditorHubApp::Initialize(ApplicationConfig_t* pAppConfig)
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

bool CEditorHubApp::Run()
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

void CEditorHubApp::Shutdown()
{
	AppFramework::ShutdownUIContext();

	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();

	m_pTools.Clear();
}

void CEditorHubApp::ConfigureRenderViews(ApplicationConfig_t* pAppConfig)
{
	pAppConfig->renderViewConfigs.Resize(2);

	// 3D Scene View.
	{
		RendererViewConfig_t sceneViewConfig{};
		sceneViewConfig.m_pszViewName = Utl::StringDuplicate("3D_Scene_View");

		// This view has 2 attachments: color and depth.
		RendererViewAttachmentConfig_t colorAttachment{};
		colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
		colorAttachment.bShouldClear = true;	// We do want to clear this target before rendering the scene.

		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
		depthAttachment.bShouldClear = true;

		// Add them to the config.
		sceneViewConfig.attachmentConfigs.Resize(2);
		sceneViewConfig.attachmentConfigs.PushBack(colorAttachment);
		sceneViewConfig.attachmentConfigs.PushBack(depthAttachment);

		sceneViewConfig.m_debuggingColor = Vector3D{ 0.3f, 0.0f, 0.7f };
		pAppConfig->renderViewConfigs.PushBack(sceneViewConfig);
	}

	// UI View.
	{
		RendererViewConfig_t UIViewConfig{};
		UIViewConfig.m_pszViewName = "UI_View";

		// This pass has 1 attachment: color.
		RendererViewAttachmentConfig_t colorAttachment{};
		colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
		colorAttachment.bShouldClear = false;

		// TEMP: Adding in a depth pass for now because of renderpass compatibility reasons.
		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
		depthAttachment.bShouldClear = true;

		UIViewConfig.attachmentConfigs.PushBack(colorAttachment);
		UIViewConfig.attachmentConfigs.PushBack(depthAttachment);

		UIViewConfig.m_debuggingColor = Vector3D{ 0.7f, 0.0f, 0.9f };
		pAppConfig->renderViewConfigs.PushBack(UIViewConfig);
	}
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
