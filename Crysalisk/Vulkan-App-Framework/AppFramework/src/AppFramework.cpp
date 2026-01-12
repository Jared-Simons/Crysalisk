//=============================================================================
// Author: Jared Simons
// Date Created: 1/11/2026
// 
// Purpose: Interface for the Application Framework.
//=============================================================================
#include "AppFramework.h"

#include <Core/Logging.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppFramework
{
	class CAppFramework : public IAppFramework
	{
	public:
		virtual bool	Initialize(AppFrameworkConfig_t& pConfig) override;

	private:
		// Pointers to engine subsystems we need.
		Renderer*		m_pRenderer{ nullptr };
		IShaderSystem*	m_pShaderSystem{ nullptr };

		struct AppRenderContext_t
		{
			ShaderHandle_t	m_hUIShader{ kInvalidID_U32 };
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
			//m_RenderState.m_hUIShader = m_pShaderSystem->AcquireShader("UI-Framework");
			// TODO: DO NOT HARDCODE THIS PATH!!!!!
			m_RenderState.m_hUIShader = m_pShaderSystem->AcquireShader("../../Vulkan-App-Framework/Assets/Shaders/UI-Framework");
		}

		return true;
	}

} // namespace AppFramework
