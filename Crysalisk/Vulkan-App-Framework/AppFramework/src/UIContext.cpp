#include "UIContext.h"

#include "ControlFactory.h"

#include <Containers/CVector.h>
#include <Platform/Platform.h>

#include "Controls/BasePanel.h"
#include "Math/MathLib.h"
#include "Renderer/IRendererBackend.h"

bool g_bQuitApplication = false;

namespace AppFramework
{
	// TODO: Clean this struct up!
	// A callback to be used to get an opaque pointer to the renderer-backend's state.
	typedef void* (*PFN_GetRendererBackendState)();
	
	struct UIContextState
	{
		ControlFactory		controlFactory{};
		RendererView		UIView{};
		IRendererBackend*	rendererBackend{ nullptr };

		Platform::IPlatformWindow*			pActiveWindow{ nullptr };
		
		// TODO: Separate input context!
		int				m_MouseXPos{ 0 }, m_MouseYPos{ 0 };
		bool			m_LeftMouseButtonPressed{ false };

		Utl::CVector<BasePanel*>	m_Controls{};
	};

	UIContextState s_UIState{};

	bool CreateUIContext(AppFramework::UI_App_Config_t& pConfig)
	{
		s_UIState.pActiveWindow = CreatePlatformWindow(pConfig.pWindow, true);
		s_UIState.pActiveWindow->SetMouseMoveEventCallback(SetMousePosition);
		s_UIState.pActiveWindow->SetMouseButtonEventCallback(SetLeftMousePressed);

		// TODO: Move this to it's own function?
		{
			// TODO: See if we can get this handed our way from the engine?
			// Load up the Vulkan renderer lib.
			Platform::ISysModule* pVulkanRendererDLL = Platform::LoadSysModule("Renderer_Vulkan.dll");
			if (!pVulkanRendererDLL)
			{
				Platform::DisplayMessageBox(s_UIState.pActiveWindow, "Failed to load the Vulkan Renderer DLL!", "Crysalisk Error!", false);
				return false;
			}

			PFN_GetRendererBackendState pfnGetRenderer = (PFN_GetRendererBackendState)pVulkanRendererDLL->GetFuncAddr("GetRendererBackendState");
			if (pfnGetRenderer)
			{
				s_UIState.rendererBackend = (IRendererBackend*)pfnGetRenderer();
			}

			// Initialize the renderer.
			RendererConfig_t rendererConfig{};
			rendererConfig.MaxPipelines = 255;
			rendererConfig.MaxTextures = 4999;
			rendererConfig.pMainWindow = s_UIState.pActiveWindow;
			rendererConfig.pRendererBackendDLL = pVulkanRendererDLL;
			rendererConfig.szApplicationName = pConfig.pWindow.pszName;
			if (!s_UIState.rendererBackend->RendererBackend_Initialize(rendererConfig))
			{
				LOG_ERROR("Failed to initialize renderer!");
				return false;
			}

			// Renderer views.
			// UI View.
			{
				RendererViewConfig_t sceneViewConfig{};
				sceneViewConfig.m_pszViewName = "3D_Scene_View";

				// This pass has 2 attachments: color and depth.
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

				s_UIState.UIView.m_debuggingColor = Vector3D{ 0.3f, 0.0f, 0.7f };

				s_UIState.rendererBackend->CreateRendererViewInternal(sceneViewConfig, s_UIState.UIView);
			}
		}

		// CONTROLTODO: Test!
		s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(s_UIState.rendererBackend, CONTROL_TYPE_PANEL, 0, 0, 1080, 25));
		s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(s_UIState.rendererBackend, CONTROL_TYPE_BUTTON, 0, 100, 1080, 25));

		return true;
	}

	void ShutdownUIContext()
	{
	}

	void RunUIContext()
	{
		// TODO: Do we need to call this if it's already being called in the Core.dll??
		//Platform::ProcessMessages();

		RenderPacket_t packet{};
		s_UIState.rendererBackend->BeginFrame(packet);
		s_UIState.rendererBackend->EndFrame(packet);

		/*for (int iControl = 0; iControl < s_UIState.m_Controls.Count(); iControl++)
		{
			s_UIState.m_Controls[iControl]->Render();
		}


		int xPos, yPos;
		GetMousePosition(xPos, yPos);
		for (int iControl = 0; iControl < s_UIState.m_Controls.Count(); iControl++)
		{
			if (s_UIState.m_Controls[iControl]->IsMouseOverlapping(xPos, yPos) && s_UIState.m_LeftMouseButtonPressed)
			{
				s_UIState.m_Controls[iControl]->OnPressed();
			}
		}

		// Need to do an extra check here in case a shutdown was triggered here.
		// TODO: This should happen prior to rendering.
		if (!ShouldRunUIContext())
		{
			return;
		}*/
	}

	bool ShouldRunUIContext()
	{
		return !g_bQuitApplication;
	}

	void SetUIContextQuitting(bool bQuitting)
	{
		g_bQuitApplication = bQuitting;
	}

	void SetMousePosition(int xPos, int yPos)
	{
		s_UIState.m_MouseXPos = xPos;
		s_UIState.m_MouseYPos = yPos;
	}
	
	void GetMousePosition(int& pXPos, int& pYPos)
	{
		pXPos = s_UIState.m_MouseXPos;
		pYPos = s_UIState.m_MouseYPos;
	}

	void SetLeftMousePressed(unsigned int mouseButtonFlags, bool bPressed)
	{
		s_UIState.m_LeftMouseButtonPressed = bPressed;
	}
}
