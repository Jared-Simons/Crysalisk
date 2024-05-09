//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Systems/IRenderer.h"

#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Vulkan/VulkanBackend.h"


class CRenderer : public IRenderer
{
	virtual bool	Renderer_Initialize() override;
	virtual void	Renderer_Shutdown() override;
	virtual bool	Renderer_Update() override;

private:
	IRendererBackend* m_RendererBackend{ nullptr };
};

bool CRenderer::Renderer_Initialize()
{
	// TODO: This currently only supports Vulkan...
	m_RendererBackend = GetVulkanBackend();

	if (m_RendererBackend != nullptr)
	{
		if (!m_RendererBackend->RendererBackend_Initialize())
		{
			LOG_FATAL("Failed to initialize rendering backend! Application cannot continue.");
			return false;
		}
	}
	else
	{
		LOG_FATAL("No rendering backend was provided, application cannot continue.");
		return false;
	}

	LOG_INFO("Renderer Initialized!");
	return true;
}

void CRenderer::Renderer_Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
	m_RendererBackend->RendererBackend_Shutdown();
	m_RendererBackend = nullptr;
}

bool CRenderer::Renderer_Update()
{
	m_RendererBackend->RendererBackend_Update();

	return true;
}

static CRenderer s_RendererFrontend;
IRenderer* gpRenderer = &s_RendererFrontend;
