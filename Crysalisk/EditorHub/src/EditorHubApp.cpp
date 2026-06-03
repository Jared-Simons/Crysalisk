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
#include <Renderer/Renderer.h>
#include <Strings/String_Utils.h>

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

	// NOTE: No camera here for now.
	virtual Camera_t* GetMainCamera() override { CAssert(0); return nullptr; }
private:
	void			ConfigureRenderViews(ApplicationConfig_t* pAppConfig);

	virtual bool LoadTool(const char* pszToolLibraryName);

private:
	ApplicationConfig_t*			m_pApplicationConfig{ nullptr };
	AppFramework::IAppFramework*	m_pAppFramework{ nullptr };
	Utl::CVector<IToolApp*>			m_pTools{};
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

typedef void* (*PFN_GetAppFrameworkState)();

bool CEditorHubApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Launching Crysalisk Tools");
	m_pTools.Resize(1);

	// Load the App Framework module.
	Platform::ISysModule* pUIModule = Platform::LoadSysModule("AppFramework");
	if (!pUIModule)
	{
		LOG_FATAL("Failed to load AppFramework.dll! Application cannot continue.");
		return false;
	}

	PFN_GetAppFrameworkState pfnGetAppFramework = (PFN_GetAppFrameworkState)pUIModule->GetFuncAddr("GetAppFrameworkState");
	if (!pfnGetAppFramework)
	{
		LOG_ERROR("Failed to get a pointer to the app framework state. Initialization failed.");
		return false;
	}
	m_pAppFramework = (AppFramework::IAppFramework*)pfnGetAppFramework();
	CAssert(m_pAppFramework != nullptr);

	// Initialize the framework.
	AppFramework::AppFrameworkConfig_t appConfig{};
	appConfig.pEngine = pAppConfig->pEngine;
	if (!m_pAppFramework->Initialize(appConfig))
	{
		LOG_ERROR("Failed to initialize app framework! Application cannot continue.");
		return false;
	}

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Run()
{
	// Serve our tools.
	// EDITORTODO: Only run the active tool!
	for (int iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Run();
	}

	m_pAppFramework->Run();

	RenderPacket_t packet{};
	m_pApplicationConfig->pEngine->GetEngineInterfaces()->pRenderer->Update(m_pAppFramework->GetActiveWindow(), packet, nullptr);

	return true;
}

void CEditorHubApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
}

bool CEditorHubApp::Render()
{
	return true;
}

void CEditorHubApp::Shutdown()
{
	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();
	m_pTools.Clear();

	m_pAppFramework->Shutdown();
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

		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

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

		// TEMP: Adding in a depth pass for now because of renderpass compatibility reasons.
		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

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
