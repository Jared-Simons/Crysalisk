//=============================================================================
// Author: Jared Simons
// Date Created: 1/11/2026
// 
// Purpose: Interface for the Application Framework.
//=============================================================================
#include "AppFramework.h"

#include <Core/Logging.h>
#include <Platform/Platform.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppFramework
{
	class CAppFramework : public IAppFramework
	{
	public:
		virtual bool	Initialize(AppFrameworkConfig_t& pConfig) override;
		virtual bool	Run() override;
		virtual void	Shutdown() override;

		virtual Platform::IPlatformWindow*	GetActiveWindow() const override { return m_RenderState.m_pWindow; }

	private:
		// Pointers to engine subsystems we need.
		IRenderer*		m_pRenderer{ nullptr };
		IShaderSystem*	m_pShaderSystem{ nullptr };

		struct AppRenderContext_t
		{
			Platform::IPlatformWindow*	m_pWindow{ nullptr };
			ShaderHandle_t				m_hUIShader{ kInvalidID_U32 };
		};

		AppRenderContext_t	m_RenderState{};
	};

	static CAppFramework s_AppFramework{};
	IAppFramework* g_pAppFramework = &s_AppFramework;

	//-------------------------------------------------------------------------
	// Purpose: Returns an opaque pointer to the App Framework state.
	//-------------------------------------------------------------------------
	DLL_EXPORT void* GetAppFrameworkState()
	{
		return g_pAppFramework;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	//-------------------------------------------------------------------------
	bool CAppFramework::Initialize(AppFrameworkConfig_t& pConfig)
	{
		if (!pConfig.pEngine)
		{
			LOG_ERROR("App Framework requires an instance of the engine! Initialization failed.");
			return false;
		}

		m_pRenderer = pConfig.pEngine->GetEngineSubsystems()->pRenderer;
		m_pShaderSystem = pConfig.pEngine->GetEngineSubsystems()->pShaderSystem;

		// Initialize render state.
		{
			// Create a window.
			Platform::WindowParams_t windowParams{};
			windowParams.X = 50;
			windowParams.Y = 50;
			windowParams.Width = 1080;
			windowParams.Height = 720;
			windowParams.pszName = "Crysalisk Tools";
			m_RenderState.m_pWindow = Platform::CreatePlatformWindow(windowParams, true);

			CAssert(m_RenderState.m_pWindow != nullptr);

			// Register the window with the renderer.
			// LEFTOFF: Figure out how to access this without linking at compile-time.
			m_pRenderer->RegisterWindow(*m_RenderState.m_pWindow);

			//m_RenderState.m_hUIShader = m_pShaderSystem->AcquireShader("UI-Framework");
			// TODO: DO NOT HARDCODE THIS PATH!!!!!
			m_RenderState.m_hUIShader = m_pShaderSystem->AcquireShader("../../Vulkan-App-Framework/Assets/Shaders/UI-Framework");
		}

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	//-------------------------------------------------------------------------
	void CAppFramework::Shutdown()
	{
		m_pRenderer->ReleaseWindowResources(*m_RenderState.m_pWindow);

		delete m_RenderState.m_pWindow;
		m_RenderState.m_pWindow = nullptr;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	//-------------------------------------------------------------------------
	bool CAppFramework::Run()
	{
		return true;
	}

} // namespace AppFramework
