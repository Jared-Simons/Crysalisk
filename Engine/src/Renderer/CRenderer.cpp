//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Systems/IRenderer.h"

#include "Core/Logging.h"

class CRenderer : public IRenderer
{
	virtual bool	Renderer_Initialize() override;
	virtual void	Renderer_Shutdown() override;
	virtual bool	Renderer_Update() override;
};

bool CRenderer::Renderer_Initialize()
{
	LOG_INFO("Initializing Renderer");
	return true;
}

void CRenderer::Renderer_Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
}

bool CRenderer::Renderer_Update()
{
	return true;
}

static CRenderer s_RendererFrontend;
IRenderer* gpRenderer = &s_RendererFrontend;
