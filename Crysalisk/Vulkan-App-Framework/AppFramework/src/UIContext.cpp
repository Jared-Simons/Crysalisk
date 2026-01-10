#include "UIContext.h"

#include "ControlFactory.h"

#include <Containers/CVector.h>
#include <Core/Logging.h>
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

	bool CreateUIContext(AppFramework::UIAppConfig_t& pConfig)
	{
		s_UIState.pActiveWindow = CreatePlatformWindow(pConfig.pWindow, true);
		s_UIState.pActiveWindow->SetMouseMoveEventCallback(SetMousePosition);
		s_UIState.pActiveWindow->SetMouseButtonEventCallback(SetLeftMousePressed);

		CASSERTMSG(pConfig.pRendererLibrary != nullptr, "AppFramework REQUIRES a pointer to a renderer library!");

		// Get access to the renderer's state.
		PFN_GetRendererBackendState pfnGetRendererState = (PFN_GetRendererBackendState)pConfig.pRendererLibrary->GetFuncAddr("GetRendererBackendState");
		CAssert(pfnGetRendererState != nullptr);

		s_UIState.rendererBackend = (IRendererBackend*)pfnGetRendererState();

		// CONTROLTODO: Test!
		//s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(s_UIState.rendererBackend, CONTROL_TYPE_PANEL, 0, 0, 1080, 25));
		//s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(s_UIState.rendererBackend, CONTROL_TYPE_BUTTON, 0, 100, 1080, 25));

		return true;
	}

	void ShutdownUIContext()
	{
	}

	void RunUIContext()
	{
		// TODO: Do we need to call this if it's already being called in the Core.dll??
		//Platform::ProcessMessages();

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

	Platform::IPlatformWindow* GetActiveWindow()
	{
		return s_UIState.pActiveWindow;
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
