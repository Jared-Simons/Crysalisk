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

// Engine
#include <Renderer/Renderer.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>
#include "Renderer/Editor_View.h"

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
	virtual bool Render(struct RenderPacket_t* pRenderPacket) override;
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

	AppUI::UIControlHandle_t		m_hButton_LoadLevelEditor{ kInvalidID_U32 };
	AppUI::UIControlHandle_t		m_hButtonLabel_LoadLevelEditor{ kInvalidID_U32 };

	// These need access to our members.
	friend static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags);
	friend static void ButtonEvent_LoadTool_LevelEditor(void* pListenerInst);
};

CEditorHubApp s_EditorHubApp{};

typedef IToolApp* (*PFN_Get_Tool_App_State)();

// Input handlers.
static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags);

// Console command callbacks.
static void ConsoleCommand_ExitApp(struct ConVar_t* pCommand, void* pValue);

// Button callbacks.
static void ButtonEvent_LoadTool_LevelEditor(void* pListenerInst);

bool CEditorHubApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());
	pAppConfig->pViews.PushBack(RenderViewCreate_EditorView());

	// Load up UI library.
	Platform::ISysModule* pAppUILibrary = Platform::LoadSysModule("AppUI");
	PFN_GetAppFrameworkState pfnGetPluginInterface = (PFN_GetAppFrameworkState)pAppUILibrary->GetFuncAddr("GetUIPluginInterface");
	CAssert(pfnGetPluginInterface);

	m_pUISystem = pfnGetPluginInterface();
	CAssert(m_pUISystem);
	m_pUISystem->PreInitialize(pAppConfig);

	return true;
}

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

	// Setup controls.
	// NOTE: Base resolution that UI elements are defined in is 1080x720.
	m_hButton_LoadLevelEditor = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
	m_pUISystem->SetControlPosition(m_hButton_LoadLevelEditor, 100, 100);
	m_pUISystem->SetControlDimensions(m_hButton_LoadLevelEditor, 250, 100);
	m_pUISystem->SetControlPressedCallback(m_hButton_LoadLevelEditor, ButtonEvent_LoadTool_LevelEditor, this);

	m_hButtonLabel_LoadLevelEditor = m_pUISystem->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
	m_pUISystem->SetControlParent(m_hButtonLabel_LoadLevelEditor, m_hButton_LoadLevelEditor);
	m_pUISystem->SetControlPosition(m_hButtonLabel_LoadLevelEditor, 20, 50);
	m_pUISystem->SetControlFont(m_hButtonLabel_LoadLevelEditor, "Assets/Fonts/UbuntuMono-R.ttf", 25.0f);
	m_pUISystem->SetControlText(m_hButtonLabel_LoadLevelEditor, "Open Level Editor");

	// Console.
	if (!ConsoleCreate(m_pUISystem, &m_Console))
	{
		LOG_ERROR("Failed to initialize developer console!");
		return false;
	}

	// Register console commands.
	struct ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;
	ConsoleCommandRegister(pConVar, "exit", CONVAR_VALUETYPE_NONE, ConsoleCommand_ExitApp, nullptr);
	ConsoleCommandRegister(pConVar, "quit", CONVAR_VALUETYPE_NONE, ConsoleCommand_ExitApp, nullptr);

	// Setup input handler.
	m_InputHandler.PFN_KeyEvent_Callback = Input_KeyEvent_Handler;
	m_InputHandler.pListenerInst = this;
	pAppConfig->pEngine->GetEngineInterfaces()->pInputSystem->PushInputMap(&m_InputHandler);

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
		m_pTools[iTool]->Tool_Update();
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

	// Update tools.
	// FIXME: Only doing first tool for now.
	if (m_pTools.Count() > 0)
	{
		m_pTools[0]->FramePrepare(pRenderPacket);
	}
}

bool CEditorHubApp::Render(struct RenderPacket_t* pRenderPacket)
{
	// FIXME: Only doing first tool for now.
	if (m_pTools.Count() > 0)
	{
		m_pTools[0]->Render(pRenderPacket);
	}
	return true;
}

void CEditorHubApp::Shutdown()
{
	// Shutdown tools
	for (u32 iTool = 0; iTool < m_pTools.Count(); iTool++)
	{
		m_pTools[iTool]->Tool_Shutdown();
	}

	ConsoleDestroy(m_pUISystem, &m_Console);

	m_pUISystem->DestroyUIControl(m_hButton_LoadLevelEditor);
	m_hButton_LoadLevelEditor = kInvalidID_U32;
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
	if (!m_pTools.Back()->Tool_Init(m_pApplicationConfig))
	{
		LOG_ERROR("Failed to initialize '%s'!", pszToolLibraryName);
		// FIXME: Make this clean and safe.
		return false;
	}
	m_pTools.Back()->SetCamera(&m_Camera);	// TODO: Find a better way to pass the camera around.

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

static void ButtonEvent_LoadTool_LevelEditor(void* pListenerInst)
{
	// FIXME: UI System needs a way to only send a 'button pressed' message once, regardless if it's being held down.
	static bool bLoaded = false;
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;

	LOG_INFO("LOADING LEVEL EDITOR!");

	CEditorHubApp* pApp = (CEditorHubApp*)pListenerInst;
	pApp->LoadTool("Tools_LevelEditor");

	// After launching the tool, hide the Hub UI.
	pApp->m_pUISystem->SetControlVisible(pApp->m_hButton_LoadLevelEditor, false);
	pApp->m_pUISystem->SetControlVisible(pApp->m_hButtonLabel_LoadLevelEditor, false);
}

ICrysApplication* GetApplicationState()
{
	return &s_EditorHubApp;
}
