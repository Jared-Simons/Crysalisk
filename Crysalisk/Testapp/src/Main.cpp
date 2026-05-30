//=============================================================================
// Author: Jared Simons
// Date Created: 14/2/2024
// 
// Purpose: The game executable's entrypoint.
//=============================================================================
#include <Application.h>

#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Logging.h>
#include <Renderer/Camera.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Resources/Map.h>
#include <Systems/MaterialSystem.h>

// Render views.
#include <Renderer/Views/3D_Scene_View.h>
#include <Renderer/Views/Debug_Render_View.h>
#include <Renderer/Views/UI_View.h>

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) int Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

class CGameApp : public ICrysApplication
{
public:
	virtual bool	PreInitialize(ApplicationConfig_t* pAppConfig) override;
	virtual bool	Initialize(ApplicationConfig_t* pAppConfig) override;
	virtual bool	Run() override;
	virtual bool	Render() override;
	virtual void	Shutdown() override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;

	virtual Camera_t* GetMainCamera() override { return &m_MainCamera; }

private:
	void			ConfigureRenderViews(ApplicationConfig_t* pAppConfig);

	ApplicationConfig_t*	m_pApplicationConfig{ nullptr };
	Camera_t				m_MainCamera{};
};

static CGameApp s_GameApp{};

static bool Event_WindowResized(EventData_t data);

int main(int argc, char** argv)
{
	ApplicationConfig_t appConfig{};
	appConfig.pApplication = &s_GameApp;

	Crysalisk_Main(appConfig);

	return 0;
}

bool CGameApp::PreInitialize(ApplicationConfig_t* pAppConfig)
{
	m_pApplicationConfig = pAppConfig;

	// Setup the render views.
	ConfigureRenderViews(pAppConfig);

	// Setup our render-views.
	pAppConfig->pViews.PushBack(RenderViewCreate_3DSceneView());
	pAppConfig->pViews.PushBack(RenderViewCreate_DebugView());
	pAppConfig->pViews.PushBack(RenderViewCreate_UIView());

	return true;
}

bool CGameApp::Initialize(ApplicationConfig_t* pAppConfig)
{
	LOG_DEBUG("Initializing Testapp");

	// Start listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);

	// Init camera data.
	m_MainCamera.RecalculateCameraVectors();

	return true;
}

void CGameApp::Shutdown()
{
	// Stop listening for events.
	IEventSystem* pEventSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventUnregister(eEventCategory::WINDOW_RESIZE, nullptr, Event_WindowResized);

	// Clean up lingering memory.
	m_pApplicationConfig->renderViewConfigs.Clear();
	m_pApplicationConfig->pViews.Clear();
}

bool CGameApp::Run()
{
	return true;
}

void CGameApp::FramePrepare(RenderPacket_t* pRenderPacket)
{
	IMaterialSystem*	pMaterialSystem = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pMaterialSystem;
	CMap*				pMap = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pCurrentScene;

	m_MainCamera.UpdateCameraData(m_pApplicationConfig->pEngine->GetEngineInterfaces()->pInputSystem, pRenderPacket->DeltaTime);
	pRenderPacket->pCamera = &m_MainCamera;

	if (pMap && pMap->IsMapLoaded())
	{
		// Get the frustum from the camera.
		Frustum frustum = m_MainCamera.CreateFrustumFromCamera();

		// TODO: Make this work with a frame allocator.
		// TODO: Figure out how to split out these matrices per model.

		pMap->GetRenderablesFromScene(pMaterialSystem, frustum, pRenderPacket, &pRenderPacket->iCulledGeometries);
	}
}

bool CGameApp::Render()
{
	/*
	* RENDERER CONVERSION NOTES: 
	* 
	* - The engine should handle metrics, so those will be cleared/updated over there.			[ ]
	* - The camera should be moved to the application.											[ ]
	* - The render-views should be moved to the application.									[ ]
	* - Scene data can probably remain engine-side.												[ ]
	* 
	* Code-flow:
	* - Engine calls application's render method.												[X]
	* - Application calls back into the engine to get a scene draw list.						[ ]
	* - Application then calls into the engine to render scene geometry.						[ ]
	* - The application should handle all app-configurable items								[ ]
	*	- Such as windows and render-views.														
	* - Metrics should be handled separately from this.											[ ]
	*/

	IRenderer* pRenderer = m_pApplicationConfig->pEngine->GetEngineInterfaces()->pRenderer;
	CAssert(pRenderer);
	
	return true;
}

void CGameApp::ConfigureRenderViews(ApplicationConfig_t* pAppConfig)
{
	pAppConfig->renderViewConfigs.Resize(2);

	// 3D Scene View.
	{
		RendererViewConfig_t sceneViewConfig{};
		sceneViewConfig.m_pszViewName = "3D_Scene_View";

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

static bool Event_WindowResized(EventData_t data)
{
	LOG_DEBUG("APP WINDOW RESIZED!");

	// Allow other handlers to get this event.
	return false;
}
