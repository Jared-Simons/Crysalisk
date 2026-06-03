//=============================================================================
// Author: Jared Simons
// Date Created: 5/17/2026
// 
// Purpose: Implementation of the debug rendering view.
//=============================================================================
#include "Debug_Render_View.h"

#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Systems/DebugGeometrySystem.h>
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

struct DebugTextShaderUniformLocations
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	fontAtlas{ kInvalidID_U16 };
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
	IDebugGeometrySystem*			m_pDebugGeoSystem{ nullptr };
	IShaderSystem*					m_pShaderSystem{ nullptr };

	Renderpass						m_pRenderpass{};

	ShaderHandle_t					m_hDebugLineShader{ kInvalidID_U32 };
	DebugLineShaderUniformLocations	m_DebugLineShaderUniforms{};

	ShaderHandle_t					m_hDebugTextShader{ kInvalidID_U32 };
	DebugTextShaderUniformLocations m_DebugTextShaderUniforms{};
};

bool CDebugRenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
{
	m_pRenderer = pEngineSystems->pRenderer;
	m_pDebugGeoSystem = pEngineSystems->pDebugGeometry;
	m_pShaderSystem = pEngineSystems->pShaderSystem;

	// Setup the debug view renderpass.
	RenderpassConfig_t DebugPassConfig{};
	DebugPassConfig.pszName = "Debug_Renderer";

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

	m_hDebugTextShader = m_pShaderSystem->AcquireShader("Debug.Text");
	if (m_hDebugTextShader == kInvalidID_U32)
	{
		LOG_ERROR("Failed to find 'Debug.Text' shader. Debug visualization will not render.");
		return false;
	}
	m_DebugTextShaderUniforms.projection = m_pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "projection");
	m_DebugTextShaderUniforms.view = m_pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "view");
	m_DebugTextShaderUniforms.color = m_pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "color");
	m_DebugTextShaderUniforms.fontAtlas = m_pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "font_atlas");
	m_DebugTextShaderUniforms.model = m_pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "model");

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
	m_pDebugGeoSystem->PopulateRenderPacket(pViewData->pGeometries, &pViewData->pVertexBuffer, &pViewData->pIndexBuffer);
}

void CDebugRenderView::Render(RenderPacket_t* pRenderData)
{
	DebugRenderView_ExtendedData_t* pViewData = (DebugRenderView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_DEBUG];
	if (m_pRenderer->BeginRenderpass(&m_pRenderpass))
	{
		bool bLineShaderGlobalsSetThisFrame = false;
		bool bTextShaderGlobalsSetThisFrame = false;

		for (int iGeo = 0; iGeo < pViewData->pGeometries.Count(); iGeo++)
		{
			// FIXME: Remove anything hardcoded.
			// FIXME: Make shader selection logic better here.
			GeometryInstance_t& pCurGeo = *pViewData->pGeometries[iGeo];

			if (pCurGeo.hShader == m_hDebugLineShader)
			{
				m_pShaderSystem->BindShader(m_hDebugLineShader);

				// Bind globals if this hasn't happened yet this frame.
				if (!bLineShaderGlobalsSetThisFrame)
				{
					m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.projection, 0, 0, &pRenderData->matProjection);
					m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.view, 0, 0, &pRenderData->pCamera->GetViewMatrix());
					bLineShaderGlobalsSetThisFrame = true;
				}

				m_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.model, 0, 0, &pCurGeo.matLocalToWorldTransform);
				m_pShaderSystem->BindPerInstanceUniforms(pCurGeo.hMaterial);
				m_pRenderer->DrawGeometry(pCurGeo, pViewData->pVertexBuffer, pViewData->pIndexBuffer);
			}
			else  // Text shader
			{
				m_pShaderSystem->BindShader(m_hDebugTextShader);

				// Bind globals if this hasn't happened yet this frame.
				if (!bTextShaderGlobalsSetThisFrame)
				{
					m_pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugTextShaderUniforms.projection, 0, 0, &pRenderData->matProjection);
					m_pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugTextShaderUniforms.view, 0, 0, &pRenderData->pCamera->GetViewMatrix());
					bTextShaderGlobalsSetThisFrame = true;
				}

				Matrix4x4 matTransform = pCurGeo.matLocalToWorldTransform;

				// The text needs to be rotated to always face the camera
				Vector vecTextPos{ matTransform.flData[12], matTransform.flData[13], matTransform.flData[14] };	// FIXME: Add a matrix method to get the transform from a matrix.
				Vector vecCameraPos = pRenderData->pCamera->vecPosition;
				float flAngle = M_ATAN2_F(vecTextPos.x - vecCameraPos.x, vecTextPos.z - vecCameraPos.z) * (180.0f / kPI_F);
				matTransform = Matrix4x4::RotateAboutYAxis(pCurGeo.matLocalToWorldTransform, M_DEG2RAD_F(flAngle + 180.0f)); // NOTE: The +180 degrees is to flip the text to face the right way.

				m_pShaderSystem->SetShaderUniform(pCurGeo.hShader, m_DebugTextShaderUniforms.model, 0, 0, &matTransform);
				m_pShaderSystem->BindPerInstanceUniforms(pCurGeo.hMaterial);
				m_pRenderer->DrawGeometry(pCurGeo, pViewData->pVertexBuffer, pViewData->pIndexBuffer);
			}			
		}
		m_pRenderer->EndRenderpass();
	}
}

// Factory function to create the view.
IRenderView* RenderViewCreate_DebugView()
{
	return new CDebugRenderView;
}
