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

	virtual void	BuildRenderPacket(RenderPacket_t* pRenderData);
	virtual void	Render(RenderPacket_t* pRenderData) override;

	virtual const char*			GetViewName() const override { return "Debug View"; }
	virtual const Renderpass*	GetRenderpass() const override { return &m_pRenderpass; }

private:
	IRenderer*						m_pRenderer{ nullptr };
	IShaderSystem*					m_pShaderSystem{ nullptr };

	Renderpass						m_pRenderpass{};
	RenderBuffer					m_VertexBuffer{};
	RenderBuffer					m_IndexBuffer{};

	ShaderHandle_t					m_hDebugLineShader{ kInvalidID_U32 };
	DebugLineShaderUniformLocations	m_DebugLineShaderUniforms{};
};

bool CDebugRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	m_pRenderer = pEngineSystems->pRenderer;
	m_pShaderSystem = pEngineSystems->pShaderSystem;

	// Setup the debug view renderpass.
	RenderpassConfig_t DebugPassConfig{};
	DebugPassConfig.pszName = "Debug Renderer";

	// This pass has 2 attachments: color and depth.
	// NOTE: The depth attachment is temporary, we do not actually use the contents of the depth buffer during this pass.
	RendererViewAttachmentConfig_t colorAttachment{};
	colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

	RendererViewAttachmentConfig_t depthAttachment{};
	depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

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

	// Set up geometry buffers.
	if (!m_pRenderer->CreateRenderBuffer(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Line_Vertex_Buffer", m_VertexBuffer))
	{
		LOG_ERROR("Failed to create debug vertex buffer!");
		return false;
	}

	if (!m_pRenderer->CreateRenderBuffer(sizeof(uint32_t) * 1024, eRenderBufferUsage::INDEX, true, "Debug_Renderer_Line_Index_Buffer", m_IndexBuffer))
	{
		LOG_ERROR("Failed to create debug index buffer!");
		return false;
	}

	return true;
}

void CDebugRenderView::Shutdown(IRenderer* pRenderer)
{
	m_pRenderer->DestroyRenderBuffer(m_IndexBuffer);
	m_pRenderer->DestroyRenderBuffer(m_VertexBuffer);

	pRenderer->DestroyRenderpass(&m_pRenderpass);
}

void CDebugRenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
{

}

void CDebugRenderView::Render(RenderPacket_t* pRenderData)
{
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		m_pShaderSystem->BindShader(m_hDebugLineShader);
		m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.projection, 0, 0, &pRenderData->matProjection);
		m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.view, 0, 0, &pRenderData->pCamera->GetViewMatrix());

		for (int iGeo = 0; iGeo < pRenderData->pDebugGeometries.Count(); iGeo++)
		{
			// FIXME: Remove anything hardcoded.
			GeometryInstance_t& pCurGeo = *pRenderData->pDebugGeometries[iGeo];
			//m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.model, 0, pCurGeo.iInstanceID, &pCurGeo.geometryInstance.matLocalToWorldTransform);
			m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.model, 0, 0, &pCurGeo.matLocalToWorldTransform);
			m_pShaderSystem->BindPerInstanceUniforms(0);
			m_pRenderer->DrawGeometry(pCurGeo, &m_pRenderer->GetDebugVertexBuffer(), &m_pRenderer->GetDebugIndexBuffer());
		}
		m_pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_DebugView()
{
	return new CDebugRenderView;
}
