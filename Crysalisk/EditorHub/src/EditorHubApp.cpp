//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: EDITORTODO: Documentation.
//=============================================================================
#include <Application.h>
#include <Containers/CVector.h>
#include <Core/ConVar.H>
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Renderer/Camera.h>
#include <Strings/String_Utils.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>

// UI Framework
#include <AppUI.h>
#include <DeveloperConsole.h>

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

	InputMap_t						m_InputHandler{};

	AppUI::IAppUI*					m_pUISystem{ nullptr };

	DeveloperConsole				m_Console{};

	// These need access to our members.
	friend static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags);
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

// Input handlers
static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags);

// Console command callbacks.
static void ConsoleCommand_ExitApp(struct ConVar_t* pCommand, void* pValue);

bool CEditorHubApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());

	// Load up UI library.
	Platform::ISysModule* pAppUILibrary = Platform::LoadSysModule("AppUI");
	PFN_GetAppFrameworkState pfnGetPluginInterface = (PFN_GetAppFrameworkState)pAppUILibrary->GetFuncAddr("GetUIPluginInterface");
	CAssert(pfnGetPluginInterface);

	m_pUISystem = pfnGetPluginInterface();
	CAssert(m_pUISystem);
	m_pUISystem->PreInitialize(pAppConfig);

	return true;
}

// HACK: UI test
static AppUI::UIControlHandle_t hTestFrame = kInvalidID_U32;
static AppUI::UIControlHandle_t	hTestLabel = kInvalidID_U32;
static AppUI::UIControlHandle_t	hTestLabel2 = kInvalidID_U32;
static AppUI::UIControlHandle_t	hTestButton = kInvalidID_U32;

bool CEditorHubApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("-------------------------");
	LOG_INFO("Launching Crysalisk Tools");
	LOG_INFO("-------------------------");

	m_pTools.Resize(1);

	m_pUISystem->Initialize(pAppConfig);

	// Init camera data.
	m_Camera.pWindow = m_pApplicationConfig->pEngine->GetEngineWindow();
	m_Camera.RecalculateCameraVectors();

	// HACK: UI test
	{
		// NOTE: Base resolution that UI elements are defined in is 1080x720.
		
		/*hTestFrame = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUISystem->SetControlPosition(hTestFrame, 50, 50);
		m_pUISystem->SetControlDimensions(hTestFrame, 496, 312);
		m_pUISystem->SetControlColor(hTestFrame, 255, 255, 255, 200);

		hTestButton = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
		m_pUISystem->SetControlParent(hTestButton, hTestFrame);
		m_pUISystem->SetControlPosition(hTestButton, 36, 40);
		m_pUISystem->SetControlDimensions(hTestButton, 140, 24);
		m_pUISystem->SetControlColor(hTestButton, 255, 100, 0, 255);

		hTestLabel = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUISystem->SetControlParent(hTestLabel, hTestFrame);
		m_pUISystem->SetControlPosition(hTestLabel, 36, 24);
		m_pUISystem->SetControlFont(hTestLabel, "Assets/Fonts/UbuntuMono-R.ttf", 25.0f);
		m_pUISystem->SetControlText(hTestLabel, "Resolution");

		hTestLabel2 = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUISystem->SetControlParent(hTestLabel2, hTestFrame);
		m_pUISystem->SetControlPosition(hTestLabel2, 36, 95);
		m_pUISystem->SetControlFont(hTestLabel2, "Assets/Fonts/UbuntuMono-R.ttf", 25.0f);
		m_pUISystem->SetControlText(hTestLabel2, "Aspect Ratio");*/
	}

	// Console.
	if (!ConsoleCreate(m_pUISystem, &m_Console))
	{
		LOG_ERROR("Failed to initialize developer console!");
		return false;
	}

	// Register console commands.
	//ConsoleRegisterCommand(&m_Console, "exit", ConsoleCommand_ExitApp, nullptr);
	struct ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;
	ConsoleCommandRegister(pConVar, "exit", CONVAR_VALUETYPE_NONE, ConsoleCommand_ExitApp, nullptr);

	// Setup input handler.
	m_InputHandler.PFN_KeyEvent_Callback = Input_KeyEvent_Handler;
	m_InputHandler.pListenerInst = this;
	pAppConfig->pEngine->GetEngineInterfaces()->pInputSystem->PushInputMap(&m_InputHandler);

	// EDITORTODO: For now, we're just gonna hard-code to opening the level editor.
	//LoadTool("Tools_LevelEditor");

	return true;
}

bool CEditorHubApp::Run()
{
	if (m_Console.visible)
	{
		ConsoleUpdate(m_pUISystem, &m_Console);
	}

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
	m_pUISystem->FramePrepare(pRenderPacket);

	if (!m_Console.visible)
	{
		// Don't move the camera if the UI is active.
		// TODO: This is probably not the best way to handle this.
		m_Camera.UpdateCameraData(m_pApplicationConfig->pEngine->GetEngineInterfaces()->pInputSystem, pRenderPacket->DeltaTime);
	}
	pRenderPacket->pCamera = &m_Camera;
}

bool CEditorHubApp::Render()
{
	return true;
}

void CEditorHubApp::Shutdown()
{
	ConsoleDestroy(m_pUISystem, &m_Console);

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

static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags)
{
	CEditorHubApp* pApp = (CEditorHubApp*)pListener;
	CAssert(pApp);
	if (keyCode == KEY_TILDE)
	{
		// Toggle visiblity of the dev console.
		static bool consoleVisible = false;
		consoleVisible = !consoleVisible;
		SetConsoleVisible(pApp->m_pUISystem, &pApp->m_Console, consoleVisible);

		if (consoleVisible)
		{
			// Give input focus over to the UI system.
			pApp->m_pUISystem->AcquireInputFocus();

			// Focus on the input text control.
			pApp->m_pUISystem->SetActiveControl(pApp->m_Console.hTextEntryControl);
		}
	}

	// Let other listeners get this.
	return false;
}

static void ConsoleCommand_ExitApp(struct ConVar_t* pCommand, void* pValue)
{
	IEventSystem* pEventSystem = g_pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
}

ICrysApplication* GetApplicationState()
{
	return &s_EditorHubApp;
}
