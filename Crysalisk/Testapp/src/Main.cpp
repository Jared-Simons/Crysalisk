//=============================================================================
// Author: Jared Simons
// Date Created: 14/2/2024
// 
// Purpose: The game executable's entrypoint.
//=============================================================================
#include <Application.h>

#include <Core/Logging.h>
#include <Renderer/RendererTypes.h>

class CGameApp : public ICrysApplication
{
public:
	virtual bool Application_Init(ApplicationConfig_t* pAppConfig) override;
	virtual bool Application_Run() override;
	virtual void Application_Shutdown() override;

	virtual void FramePrepare(struct RenderPacket_t* pRenderPacket) override {};

private:
	void		ConfigureRenderViews(ApplicationConfig_t* pAppConfig);
};

static CGameApp s_GameApp{};

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) int Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

int main(int argc, char** argv)
{
	ApplicationConfig_t appConfig{};
	appConfig.pApplication = &s_GameApp;

	Crysalisk_Main(appConfig);

	return 0;
}

bool CGameApp::Application_Init(ApplicationConfig_t* pAppConfig)
{
	LOG_DEBUG("Initializing Testapp");

	// Setup the render views.
	ConfigureRenderViews(pAppConfig);

	return true;
}

bool CGameApp::Application_Run()
{
	return true;
}

void CGameApp::Application_Shutdown()
{
}

void CGameApp::ConfigureRenderViews(ApplicationConfig_t* pAppConfig)
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
