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

	virtual void	Render(RenderPacket_t* pRenderData) override;

private:
	Utl::CVector<Matrix4x4>				TransformMatrices{};	// TODO: Figure out how to split out these matrices
	Utl::CVector<GeometryInstance_t*>	pOpaqueGeometries{};
	Utl::CVector<GeometryInstance_t*>	pTransparentGeometries{};

	Renderpass							pRenderpass{};
};

bool C3DSceneRenderView::Initialize(IRenderer* pRenderer)
{
	// Reserve memory for our geometry lists.
	TransformMatrices.Resize(32);
	pOpaqueGeometries.Resize(512);
	pTransparentGeometries.Resize(512);

	// Register with the renderer.
	pRenderer->RegisterRenderView(this);

	return true;
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
