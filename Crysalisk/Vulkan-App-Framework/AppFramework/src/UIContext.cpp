#include "UIContext.h"

#include "ControlFactory.h"

#include <Containers/CVector.h>
#include <Platform/Platform.h>
#include "Renderer/VulkanRenderer.h"

// CONTROLTODO: TEST!
#include "Controls/BasePanel.h"
#include "Math/MathLib.h"

bool g_bQuitApplication = false;

namespace AppFramework
{
	// TODO: Clean this struct up!
	struct UIContextState
	{
		ControlFactory	controlFactory{};

		// TODO: Store the memory in this struct.
		IVulkanRenderer*	g_pRenderer{ ::g_pRenderer };

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

		// Initialize the renderer.
		s_UIState.g_pRenderer->Initialize(s_UIState.pActiveWindow->GetInternalHandle(), 800, 600);

		// CONTROLTODO: Test!
		s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(CONTROL_TYPE_PANEL, 0, 0, 1080, 25));
		s_UIState.m_Controls.PushBack(s_UIState.controlFactory.CreateControl(CONTROL_TYPE_BUTTON, 0, 100, 1080, 25));

		return true;
	}

	void ShutdownUIContext()
	{
		g_pRenderer->Shutdown();
	}

	void RunUIContext()
	{
		Platform::ProcessMessages();

		g_pRenderer->BeginFrame();
		for (int iControl = 0; iControl < s_UIState.m_Controls.Count(); iControl++)
		{
			s_UIState.m_Controls[iControl]->Render();
		}
		g_pRenderer->EndFrame();


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
		}
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
