//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#include "DebugRenderer.h"

#include "Renderer.h"

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
class CDebugRenderer : public IDebugRenderer
{
public:
	virtual bool	Initialize() override;
	virtual void	Shutdown() override;

	virtual void	Render() override;

private:
	RendererView	m_DebugView{};

	// NOTE: Does not have an index buffer.
	RenderBuffer	m_DebugVertexBuffer{};
};

CDebugRenderer s_DebugRenderer;
IDebugRenderer* g_pDebugRenderer = &s_DebugRenderer;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CDebugRenderer::Initialize()
{
	// Setup the debug renderpass.
	RendererViewConfig_t debugViewConfig{};
	debugViewConfig.m_pszViewName = "Debug_Renderer";

	// This pass has 2 attachments: color and depth.
	// NOTE: The depth attachment is temporary, we do not actually use the contents of the depth buffer during this pass.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
	colorAttachment.bShouldClear = false;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
	depthAttachment.bShouldClear = false;

	// Add them to the config.
	debugViewConfig.attachmentConfigs.Resize(2);
	debugViewConfig.attachmentConfigs.PushBack(colorAttachment);
	debugViewConfig.attachmentConfigs.PushBack(depthAttachment);

	m_DebugView.m_debuggingColor = Vector3D{ 0.5f, 0.0f, 0.7f };

	if (!g_pRenderer->CreateRendererView(debugViewConfig, m_DebugView))
	{
		LOG_ERROR("Failed to create debug render view!");
		return false;
	}

	if (!g_pRenderer->CreateRenderBuffer(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Geometry_Buffer", m_DebugVertexBuffer))
	{
		LOG_ERROR("Failed to create debug render buffer!");
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CDebugRenderer::Shutdown()
{
	g_pRenderer->DestroyRenderBuffer(m_DebugVertexBuffer);
	g_pRenderer->DestroyRendererView(m_DebugView);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
void CDebugRenderer::Render()
{
	if (g_pRenderer->BeginRendererView(m_DebugView))
	{
		g_pRenderer->EndRendererView();
	}
	else
	{
		LOG_ERROR("Failed to begin Debug Renderpass. Skipping debug rendering.");
		return;
	}
}
