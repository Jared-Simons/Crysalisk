//=============================================================================
// Author: Jared Simons
// Date Created: 5/17/2026
// 
// Purpose: Implementation of the debug rendering view.
//=============================================================================
#include "Debug_Render_View.h"

#include <Core/Engine.h>
#include <Renderer/DebugRenderer.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

//-------------------------------------------------------------------------
// Purpose: Indices for each of the debug shader uniform lookups.
//-------------------------------------------------------------------------
struct DebugLineShaderUniformLocations
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
};

//-------------------------------------------------------------------------
// Purpose: Debug render view implementation.
//-------------------------------------------------------------------------
class CDebugRenderView : public IRenderView
{
public:
	virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
	virtual void	Shutdown(IRenderer* pRenderer) override;

	virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) override;
	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "Debug View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	IRenderer*						m_pRenderer{ nullptr };
	IDebugRenderer*					m_pDebugRenderer{ nullptr };	// FIXME: Rename this, it shouldn't be responsible for rendering.
	IShaderSystem*					m_pShaderSystem{ nullptr };

	Renderpass						m_pRenderpass{};

	ShaderHandle_t					m_hDebugLineShader{ kInvalidID_U32 };
	DebugLineShaderUniformLocations	m_DebugLineShaderUniforms{};
};

bool CDebugRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	m_pRenderer = pEngineSystems->pRenderer;
	m_pDebugRenderer = pEngineSystems->pDebugRenderer;
	m_pShaderSystem = pEngineSystems->pShaderSystem;

	// Setup the debug view renderpass.
	RenderpassConfig_t DebugPassConfig{};
	DebugPassConfig.pszName = "Debug_Renderer";

	// This pass has 2 attachments: color and depth.
	// NOTE: The depth attachment is temporary, we do not actually use the contents of the depth buffer during this pass.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
	colorAttachment.bShouldClear = false;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
	depthAttachment.bShouldClear = false;

	DebugPassConfig.attachmentConfigs.PushBack(colorAttachment);
	DebugPassConfig.attachmentConfigs.PushBack(depthAttachment);
	DebugPassConfig.clearFlags = RENDERPASS_CLEAR_NONE;
	DebugPassConfig.bPresentsAfter = false;

	if (!m_pRenderer->CreateRenderpass(&DebugPassConfig, &m_pRenderpass))
	{
		LOG_ERROR("Failed to create Debug Renderpass!");
		return false;
	}

	// Register with the renderer.
	m_pRenderer->RegisterRenderView(this);

	// Set up shaders.
	m_hDebugLineShader = m_pShaderSystem->AcquireShader("Debug.Line");
	if (m_hDebugLineShader == kInvalidID_U32)
	{
		LOG_ERROR("Failed to find 'Debug.Line' shader. Debug visualization will not render.");
		return false;
	}
	m_DebugLineShaderUniforms.projection = m_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "projection");
	m_DebugLineShaderUniforms.view = m_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "view");
	m_DebugLineShaderUniforms.color = m_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "color");
	m_DebugLineShaderUniforms.model = m_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "model");

	return true;
}

void CDebugRenderView::Shutdown(IRenderer* pRenderer)
{
	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CDebugRenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
{
	if (!pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_DEBUG])
	{
		pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_DEBUG] = new DebugRenderView_ExtendedData_t;
	}

	// Get any geometry data from the debug geo system.
	DebugRenderView_ExtendedData_t* pViewData = (DebugRenderView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_DEBUG];
	m_pDebugRenderer->PopulateRenderPacket(pViewData->pGeometries, &pViewData->pVertexBuffer, &pViewData->pIndexBuffer);
}

void CDebugRenderView::Render(RenderPacket_t* pRenderData)
{
	DebugRenderView_ExtendedData_t* pViewData = (DebugRenderView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_DEBUG];
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		m_pShaderSystem->BindShader(m_hDebugLineShader);
		m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.projection, 0, 0, &pRenderData->matProjection);
		m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.view, 0, 0, &pRenderData->pCamera->GetViewMatrix());

		for (int iGeo = 0; iGeo < pViewData->pGeometries.Count(); iGeo++)
		{
			// FIXME: Remove anything hardcoded.
			GeometryInstance_t& pCurGeo = *pViewData->pGeometries[iGeo];
			m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.model, 0, 0, &pCurGeo.matLocalToWorldTransform);
			m_pShaderSystem->BindPerInstanceUniforms(pCurGeo.hMaterial);
			m_pRenderer->DrawGeometry(pCurGeo, pViewData->pVertexBuffer, pViewData->pIndexBuffer);
		}
		m_pRenderer->EndRenderpass();
	}

	// Clear out per-frame data.
	pViewData->pVertexBuffer->m_CurrentBoundOffset = kInvalidID_U64;
	pViewData->pIndexBuffer->m_CurrentBoundOffset = kInvalidID_U64;
}

// Factory function to create the view.
IRenderView* RenderViewCreate_DebugView()
{
	return new CDebugRenderView;
}
