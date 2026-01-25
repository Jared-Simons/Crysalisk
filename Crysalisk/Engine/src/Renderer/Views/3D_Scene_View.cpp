//=============================================================================
// Author: Jared Simons
// Date Created: 1/23/2026
// 
// Purpose: Implementation of the main scene rendering view.
//=============================================================================
#include "3D_Scene_View.h"

#include <Containers/CVector.h>
#include "Core/Engine.h"
#include <Math/MathLib.h>
#include "Renderer/Renderer.h"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"

class C3DSceneRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "3D Scene View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	Utl::CVector<Matrix4x4>				m_TransformMatrices{};	// TODO: Figure out how to split out these matrices
	Utl::CVector<GeometryInstance_t*>	m_pOpaqueGeometries{};
	Utl::CVector<GeometryInstance_t*>	m_pTransparentGeometries{};

	Renderpass							m_pRenderpass{};
	IRenderer*							m_pRenderer{ nullptr };
	IMaterialSystem*					m_pMaterialSystem{ nullptr };
	IShaderSystem*						m_pShaderSystem{ nullptr };
};

bool C3DSceneRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	CAssert(pEngineSystems != nullptr);

	// Reserve memory for our geometry lists.
	m_TransformMatrices.Resize(32);
	m_pOpaqueGeometries.Resize(512);
	m_pTransparentGeometries.Resize(512);

	// Store off subsystem pointers.
	m_pRenderer = pEngineSystems->pRenderer;
	m_pMaterialSystem = pEngineSystems->pMaterialSystem;
	m_pShaderSystem = pEngineSystems->pShaderSystem;

	// Setup the renderpass.
	RenderpassConfig_t scenePassConfig{};
	scenePassConfig.pszName = "3D Scene View";

	// This view renders to 2 attachments: color and depth.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

	scenePassConfig.attachmentConfigs.PushBack(colorAttachment);
	scenePassConfig.attachmentConfigs.PushBack(depthAttachment);
	scenePassConfig.clearFlags = RENDERPASS_CLEAR_COLOR | RENDERPASS_CLEAR_DEPTH | RENDERPASS_CLEAR_STENCIL;
	scenePassConfig.bPresentsAfter = true;

	pEngineSystems->pRenderer->CreateRenderpass(&scenePassConfig, &m_pRenderpass);

	// Register with the renderer.
	pEngineSystems->pRenderer->RegisterRenderView(this);

	return true;
}

void C3DSceneRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void C3DSceneRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{

	}
	m_pRenderer->EndRenderpass();
}

// Factory function to create the view.
IRenderView* RendererViewCreate_3DSceneView()
{
	C3DSceneRenderView* pNewView = new C3DSceneRenderView;
	return pNewView;
}
