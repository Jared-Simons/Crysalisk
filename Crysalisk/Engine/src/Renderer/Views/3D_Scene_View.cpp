//=============================================================================
// Author: Jared Simons
// Date Created: 1/23/2026
// 
// Purpose: Implementation of the main scene rendering view.
//=============================================================================
#include "3D_Scene_View.h"

#include <Containers/CVector.h>
#include "Core/Engine.h"
#include "Renderer/Renderer.h"
#include "Resources/Map.h"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"

class C3DSceneRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) override;
	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "3D Scene View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	RenderBuffer*						m_pWorldVertexBuffer{ nullptr };
	RenderBuffer*						m_pWorldIndexBuffer{ nullptr };

	Renderpass							m_pRenderpass{};
	IRenderer*							m_pRenderer{ nullptr };
	IMaterialSystem*					m_pMaterialSystem{ nullptr };
	IShaderSystem*						m_pShaderSystem{ nullptr };
};

bool C3DSceneRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	CAssert(pEngineSystems != nullptr);

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
	scenePassConfig.bPresentsAfter = false;

	pEngineSystems->pRenderer->CreateRenderpass(&scenePassConfig, &m_pRenderpass);

	// Get our renderbuffers.
	m_pWorldVertexBuffer = m_pRenderer->GetRenderBufferByName("World_Vertex_Buffer");
	if (m_pWorldVertexBuffer == nullptr)
	{
		LOG_ERROR("3D Scene View failed to initialize!");
		return false;
	}

	m_pWorldIndexBuffer = m_pRenderer->GetRenderBufferByName("World_Index_Buffer");
	if (m_pWorldIndexBuffer == nullptr)
	{
		LOG_ERROR("3D Scene View failed to initialize!");
		return false;
	}

	// Register with the renderer.
	pEngineSystems->pRenderer->RegisterRenderView(this);

	return true;
}

void C3DSceneRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void C3DSceneRenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
{
	if (!pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE])
	{
		pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE] = new MainSceneView_ExtendedData_t;
		MainSceneView_ExtendedData_t* pViewData = (MainSceneView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE];
		
		pViewData->pOpaqueGeometries.SetAllocator(pRenderData->pFrameAllocator);
		pViewData->pTransparentGeometries.SetAllocator(pRenderData->pFrameAllocator);

		// TODO: Is this a hard limit? If so, can we change this based on how much geometry is in the scene or make it a little more dynamic?
		pViewData->pOpaqueGeometries.Resize(8999);
		pViewData->pTransparentGeometries.Resize(8999);
	}

	CMap*							pMap = g_pEngine->GetEngineInterfaces()->pCurrentScene;
	MainSceneView_ExtendedData_t*	pViewData = (MainSceneView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE];

	if (pMap && pMap->IsMapLoaded())
	{
		// Get the frustum from the camera.
		Frustum frustum = pRenderData->pCamera->CreateFrustumFromCamera();

		pMap->GetRenderablesFromScene(g_pEngine->GetEngineInterfaces()->pMaterialSystem, frustum,
									  pViewData->pOpaqueGeometries, pViewData->pTransparentGeometries,
									  &pRenderData->pMetrics->NumGeometriesFrustumCulled);
	}
}

void C3DSceneRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		MainSceneView_ExtendedData_t* pViewData = (MainSceneView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE];
		Utl::CVector<GeometryInstance_t*>& pOpaqueGeometries = pViewData->pOpaqueGeometries;
		Utl::CVector<GeometryInstance_t*>& pTransparentGeometries = pViewData->pTransparentGeometries;

		m_pMaterialSystem->ApplyGlobalRenderData(pRenderData->matProjection, pRenderData->pCamera->GetViewMatrix(), pRenderData->pCamera->vecPosition, pRenderData->iDebugRendererMode);

		// Render opaque geometries.
		for (i32 iGeo = 0; iGeo < pOpaqueGeometries.Count(); iGeo++)
		{
			// Apply transformations.
			m_pMaterialSystem->ApplyPerDrawRenderData(pOpaqueGeometries[iGeo]->matLocalToWorldTransform);

			// Bind the appropriate material.
			m_pMaterialSystem->ApplyMaterialInstance(pOpaqueGeometries[iGeo]->hShaderInstance);

			// Draw the sucker!
			m_pRenderer->DrawGeometry(*pOpaqueGeometries[iGeo], m_pWorldVertexBuffer, m_pWorldIndexBuffer);

			pRenderData->pMetrics->NumDrawCalls++;
		}

		// Render transparent geometries.
		for (i32 iGeo = 0; iGeo < pTransparentGeometries.Count(); iGeo++)
		{
			// Apply transformations.
			m_pMaterialSystem->ApplyPerDrawRenderData(pTransparentGeometries[iGeo]->matLocalToWorldTransform);

			// Bind the appropriate material.
			m_pMaterialSystem->ApplyMaterialInstance(pTransparentGeometries[iGeo]->hShaderInstance);

			// Draw the sucker!
			m_pRenderer->DrawGeometry(*pTransparentGeometries[iGeo], m_pWorldVertexBuffer, m_pWorldIndexBuffer);

			pRenderData->pMetrics->NumDrawCalls++;
		}
		m_pRenderer->EndRenderpass();

		pViewData->pOpaqueGeometries.Clear();
		pViewData->pTransparentGeometries.Clear();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_3DSceneView()
{
	C3DSceneRenderView* pNewView = new C3DSceneRenderView;
	return pNewView;
}
