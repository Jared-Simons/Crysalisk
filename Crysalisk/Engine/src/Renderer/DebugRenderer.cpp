//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#include "DebugRenderer.h"

#include <Containers/CVector.h>
#include <Math/VectorTypes.h>
#include "Renderer.h"
#include <Strings/String_Utils.h>
#include "Systems/ShaderSystem.h"

// DEBUGLEFTOFF:
// - We do not need all of the details of the GeometryInstance_t struct for rendering lines and whatnot.
// - We should simplify this significantly.

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
struct DebugGeometry_t
{
	unsigned int		iInstanceID{ kInvalidID_U32 };
	Vector4D			color{}; // DEBUGTODO: See if we have to store this off (if the color is constant, could we just write to the descriptor and forget?)
	GeometryInstance_t	geometryInstance{};
};

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
struct DebugShaderUniformLocations
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
};

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
class CDebugRenderer : public IDebugRenderer
{
public:
	virtual bool	Initialize() override;
	virtual bool	InitializeDebugShaders() override;
	virtual void	Shutdown() override;

	virtual void	Render(DebugRendererFrameData_t& pFrameData) override;

	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color);

private:
	int				GetFirstFreeGeometryDataIndex() const;

private:
	RendererView	m_DebugView{};

	// NOTE: Does not have an index buffer.
	RenderBuffer		m_DebugVertexBuffer{};

	ShaderHandle_t				m_hDebugLineShader{ kInvalidID_U32 };
	DebugShaderUniformLocations	m_DebugShaderUniforms{};

	int					m_iNumGeometryInstances{ 0 };
	DebugGeometry_t*	m_GeometryData{ nullptr };
};

CDebugRenderer s_DebugRenderer;
IDebugRenderer* g_pDebugRenderer = &s_DebugRenderer;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CDebugRenderer::Initialize()
{
	// Setup debug geometry array.
	m_iNumGeometryInstances = 64; // DEBUGTODO: Make this configurable.
	m_GeometryData = new DebugGeometry_t[m_iNumGeometryInstances]; 

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
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
bool CDebugRenderer::InitializeDebugShaders()
{
	m_hDebugLineShader = g_pShaderSystem->AcquireShader("Debug.Line");

	m_DebugShaderUniforms.projection = g_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "projection");
	m_DebugShaderUniforms.view = g_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "view");
	m_DebugShaderUniforms.color = g_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "color");
	m_DebugShaderUniforms.model = g_pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "model");

	// DEBUGTODO: Test
	{
		Vector3D ptStartPos{ -90, 94, -178 };
		Vector3D ptEndPos{ 470, 94, -178 };
		Vector4D color{ 1.0f, 0.0f, 0.0f, 1.0f };
		AddLine(ptStartPos, ptEndPos, color);
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

	// Release debug geometry data.
	delete[] m_GeometryData;
	m_GeometryData = nullptr;
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
void CDebugRenderer::Render(DebugRendererFrameData_t& pFrameData)
{
	if (g_pRenderer->BeginRendererView(m_DebugView))
	{
		// DEBUGTODO: Test
		{
			if (m_GeometryData[0].iInstanceID != kInvalidID_U32)
			{
				DebugGeometry_t& pTestGeometry = m_GeometryData[0];
				g_pShaderSystem->BindShader(m_hDebugLineShader);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.projection, 0, 0, &pFrameData.matProjection);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.view, 0, 0, &pFrameData.matView);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.color, 0, pTestGeometry.iInstanceID, &pTestGeometry.color);

				// DEBUGTODO: Get this from somewhere!
				Matrix4x4 matModel{ 1.0f };
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.model, 0, pTestGeometry.iInstanceID, &matModel);

				g_pShaderSystem->BindPerFrameUniforms();
				g_pShaderSystem->BindPerInstanceUniforms(pTestGeometry.iInstanceID);
				g_pRenderer->DrawGeometry(pTestGeometry.geometryInstance, &m_DebugVertexBuffer, nullptr);
			}
		}

		g_pRenderer->EndRendererView();
	}
	else
	{
		LOG_ERROR("Failed to begin Debug Renderpass. Skipping debug rendering.");
		return;
	}
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
int CDebugRenderer::AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;

	// Acquire new shader instance resources.
	g_pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugLineInstance");

	Vector3D vecPositions[2] = { ptStartPos, ptEndPos };
	g_pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, 2, sizeof(Vector3D), vecPositions, 0, nullptr, pNewGeometry.geometryInstance);

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
int CDebugRenderer::GetFirstFreeGeometryDataIndex() const
{
	for (int idx = 0; idx < m_iNumGeometryInstances; idx++)
	{
		if (m_GeometryData[idx].iInstanceID == kInvalidID_U32)
		{
			// Found an unused index!
			return idx;
		}
	}

	// DEBUGTODO: Logging!
	return kInvalidID_U32;
}
