//=============================================================================
// Author: Jared Simons
// Date Created: 1/23/2026
// 
// Purpose: Implementation of the main scene rendering view.
//=============================================================================
#include "3D_Scene_View.h"

#include <Containers/CVector.h>
#include <Math/MathLib.h>
#include "Renderer/Renderer.h"

class C3DSceneRenderView : public IRenderView
{
public:
	virtual bool	Initialize(IRenderer* pRenderer) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	Render(RenderPacket_t* pRenderData) override;

private:
	Utl::CVector<Matrix4x4>				m_TransformMatrices{};	// TODO: Figure out how to split out these matrices
	Utl::CVector<GeometryInstance_t*>	m_pOpaqueGeometries{};
	Utl::CVector<GeometryInstance_t*>	m_pTransparentGeometries{};

	Renderpass							m_pRenderpass{};
};

bool C3DSceneRenderView::Initialize(IRenderer* pRenderer)
{
	// Reserve memory for our geometry lists.
	m_TransformMatrices.Resize(32);
	m_pOpaqueGeometries.Resize(512);
	m_pTransparentGeometries.Resize(512);

	// Setup the renderpass.
	RenderpassConfig_t scenePassConfig{};

	// This view renders to 2 attachments: color and depth.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

	scenePassConfig.attachmentConfigs.PushBack(colorAttachment);
	scenePassConfig.attachmentConfigs.PushBack(depthAttachment);
	scenePassConfig.clearFlags = RENDERPASS_CLEAR_COLOR | RENDERPASS_CLEAR_DEPTH | RENDERPASS_CLEAR_STENCIL;

	pRenderer->CreateRenderpass(&scenePassConfig, &m_pRenderpass);

	// Register with the renderer.
	pRenderer->RegisterRenderView(this);

	return true;
}

void C3DSceneRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void C3DSceneRenderView::Render(RenderPacket_t* pRenderData)
{

}

// Factory function to create the view.
IRenderView* RendererViewCreate_3DSceneView()
{
	C3DSceneRenderView* pNewView = new C3DSceneRenderView;
	return pNewView;
}
