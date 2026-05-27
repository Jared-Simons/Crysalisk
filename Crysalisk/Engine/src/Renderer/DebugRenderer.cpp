//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#include "DebugRenderer.h"

#include <Containers/CVector.h>
#include <Math/MathLib.h>
#include "Renderer.h"
#include <Renderer/Camera.h>
#include <Strings/String_Utils.h>
#include "Systems/FontSystem.h"
#include "Systems/ShaderSystem.h"

//-------------------------------------------------------------------------
// Purpose: Defines different types of geometry that require different rendering behaviors.
//-------------------------------------------------------------------------
enum class DebugGeometryType
{
	UNDEFINED = 0,
	// Geometry is rendered with lines.
	LINE,
	// Geometry is also rendered with lines, but needs to follow the camera.
	CAMERA_FRUSTUM,
	// Geometry is rendered as 2D glyphs in the 3D scene.
	TEXT,

	MAX_TYPES
};

//-------------------------------------------------------------------------
// Purpose: An instance of debug geometry, contains all of the
//			data/resources necessary to render debug geometry.
//-------------------------------------------------------------------------
struct DebugGeometry_t
{
	unsigned int		iInstanceID{ kInvalidID_U32 };
	Vector4D			color{};
	GeometryInstance_t	geometryInstance{};
	DebugGeometryType	eType{ DebugGeometryType::UNDEFINED };
	float				flTimeToRemove{ 0.0f };
};

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
// Purpose: Debug renderer implementation.
//-------------------------------------------------------------------------
class CDebugRenderer : public IDebugRenderer
{
public:
	virtual bool	Initialize(DebugRendererConfig_t& pAppConfig) override;
	virtual bool	InitializeDebugShaders() override;
	virtual void	Shutdown() override;

	virtual void	Update() override;
	virtual void	Render(DebugRendererFrameData_t& pFrameData) override;

	virtual int		AddPoint(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) override;
	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, float flDuration) override;
	virtual int		AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration) override;
	virtual int		AddBBox(Vector vecMins, Vector vecMaxs, Vector4D color, float flDuration) override;
	virtual int		AddSphere(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) override;
	virtual int		AddText(class IFontSystem* pFontSystem, Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, float flDuration) override;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color, float flDuration) override;
	virtual void	RemoveDebugGeometry(int debugGeoHandle) override;

private:
	int				GetFirstFreeGeometryDataIndex() const;

private:
	DebugRendererConfig_t		m_Config{};

	// The debug renderer's renderpass.
	RendererView				m_DebugView{};

	RenderBuffer				m_DebugVertexBuffer{};
	RenderBuffer				m_DebugIndexBuffer{};

	ShaderHandle_t					m_hDebugLineShader{ kInvalidID_U32 };
	DebugLineShaderUniformLocations	m_DebugLineShaderUniforms{};

	ShaderHandle_t					m_hDebugTextShader{ kInvalidID_U32 };
	DebugTextShaderUniformLocations	m_DebugTextShaderUniforms{};

	int					m_iNumGeometryInstances{ 0 };
	DebugGeometry_t*	m_GeometryData{ nullptr };

	// NOTE: This gets updated from the main renderer's clock.
	float			m_flRendererTime{ 0.0f };
};

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IDebugRenderer::AllocateDebugRendererState(IDebugRenderer** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold debug render system state was not provided! Debug render system creation failed!");
		return;
	}

	*ppSystemState = new CDebugRenderer;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CDebugRenderer::Initialize(DebugRendererConfig_t& pAppConfig)
{
	// Save off config.
	m_Config = pAppConfig;

	// Setup debug geometry array.
	m_iNumGeometryInstances = pAppConfig.uMaxDebugGeometries;
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

	// TODO: Integrate with the new RenderView system.
	/*if (!m_Config.pRenderer->CreateRendererView(debugViewConfig, m_DebugView))
	{
		LOG_ERROR("Failed to create debug render view!");
		return false;
	}*/

	/*if (!m_Config.pRenderer->CreateRenderBuffer(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Line_Vertex_Buffer", m_DebugVertexBuffer))
	{
		LOG_ERROR("Failed to create debug vertex buffer!");
		return false;
	}*/

	/*if (!m_Config.pRenderer->CreateRenderBuffer(sizeof(uint32_t) * 1024, eRenderBufferUsage::INDEX, true, "Debug_Renderer_Line_Index_Buffer", m_DebugIndexBuffer))
	{
		LOG_ERROR("Failed to create debug index buffer!");
		return false;
	}*/

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Perform some post-initialization setup.
//-------------------------------------------------------------------------
bool CDebugRenderer::InitializeDebugShaders()
{
	IShaderSystem* pShaderSystem = m_Config.pShaderSystem;
	m_hDebugLineShader = pShaderSystem->AcquireShader("Debug.Line");

	m_DebugLineShaderUniforms.projection = pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "projection");
	m_DebugLineShaderUniforms.view = pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "view");
	m_DebugLineShaderUniforms.color = pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "color");
	m_DebugLineShaderUniforms.model = pShaderSystem->GetShaderUniformLocation(m_hDebugLineShader, "model");

	m_hDebugTextShader = pShaderSystem->AcquireShader("Debug.Text");

	m_DebugTextShaderUniforms.projection = pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "projection");
	m_DebugTextShaderUniforms.view = pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "view");
	m_DebugTextShaderUniforms.color = pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "color");
	m_DebugTextShaderUniforms.fontAtlas = pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "font_atlas");
	m_DebugTextShaderUniforms.model = pShaderSystem->GetShaderUniformLocation(m_hDebugTextShader, "model");

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CDebugRenderer::Shutdown()
{
	//m_Config.pRenderer->DestroyRenderBuffer(m_DebugIndexBuffer);
	//m_Config.pRenderer->DestroyRenderBuffer(m_DebugVertexBuffer);

	// Release debug geometry data.
	delete[] m_GeometryData;
	m_GeometryData = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Updates internal debug renderer state.
//-------------------------------------------------------------------------
void CDebugRenderer::Update()
{
	for (int iGeo = 0; iGeo < m_iNumGeometryInstances; iGeo++)
	{
		DebugGeometry_t& pCurGeo = m_GeometryData[iGeo];
		if (pCurGeo.iInstanceID == kInvalidID_U32)
			continue;

		if (pCurGeo.flTimeToRemove < 0.0f)
			continue;

		if (pCurGeo.flTimeToRemove < m_flRendererTime)
		{
			// Time to remove this geo.
			RemoveDebugGeometry(iGeo);
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Render all active debug geometries.
//-------------------------------------------------------------------------
void CDebugRenderer::Render(DebugRendererFrameData_t& pFrameData)
{
	m_flRendererTime = pFrameData.flCurTime;

	// TODO: This is borked!
	// Need to be integrated with the new RenderView system.
	/*if (m_Config.pRenderer->BeginRendererView(m_DebugView))
	{
		IShaderSystem* pShaderSystem = m_Config.pShaderSystem;

		// Apply per-frame shader uniforms.
		pShaderSystem->BindShader(m_hDebugLineShader);
		pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.projection, 0, 0, &pFrameData.matProjection);
		pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.view, 0, 0, &pFrameData.matView);

		pShaderSystem->BindShader(m_hDebugTextShader);
		pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugLineShaderUniforms.projection, 0, 0, &pFrameData.matProjection);
		pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugLineShaderUniforms.view, 0, 0, &pFrameData.matView);

		bool bLineShaderBound = false;
		ShaderHandle_t hCurrentlyBoundShader = m_hDebugTextShader;

		// Just iterate through everything for now.
		// This is not very efficient, could make this work with a spatial partition or tie to entity/pvs.
		for (int iGeo = 0; iGeo < m_iNumGeometryInstances; iGeo++)
		{
			DebugGeometry_t& pCurGeo = m_GeometryData[iGeo];
			if (pCurGeo.iInstanceID == kInvalidID_U32)
				continue;

			// We need to save off the transform and restore it after rendering.
			Matrix4x4 matSave = pCurGeo.geometryInstance.matLocalToWorldTransform;

			if (pCurGeo.eType == DebugGeometryType::TEXT)
			{
				if (bLineShaderBound)
				{
					pShaderSystem->BindShader(m_hDebugTextShader);
					hCurrentlyBoundShader = m_hDebugTextShader;
					bLineShaderBound = false;
				}
				// The text needs to be rotated to always face the camera (billboarding!)
				Vector vecTextPos{ matSave.flData[12], matSave.flData[13], matSave.flData[14] };
				Vector vecCameraPos{ pFrameData.vecCameraPosition };
				float flAngle = M_ATAN2_F(vecTextPos.x - vecCameraPos.x, vecTextPos.z - vecCameraPos.z) * (180.0f / kPI_F);
				matSave = Matrix4x4::RotateAboutYAxis(matSave, M_DEG2RAD_F(flAngle + 180.0f));	// NOTE: The +180 is to flip the text to face the right way.
				pCurGeo.geometryInstance.matLocalToWorldTransform = matSave;
			}
			else
			{
				if (!bLineShaderBound)
				{
					pShaderSystem->BindShader(m_hDebugLineShader);
					hCurrentlyBoundShader = m_hDebugLineShader;
					bLineShaderBound = true;
				}

				if (pCurGeo.eType == DebugGeometryType::CAMERA_FRUSTUM)
				{
					// Parent this to the camera's (inverse) transform.
					pCurGeo.geometryInstance.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);
					pCurGeo.geometryInstance.matLocalToWorldTransform = pCurGeo.geometryInstance.matLocalToWorldTransform * pFrameData.matProjection.Inverse();
					pCurGeo.geometryInstance.matLocalToWorldTransform = pCurGeo.geometryInstance.matLocalToWorldTransform * pFrameData.matView.Inverse();
				}
			}

			if (pCurGeo.eType == DebugGeometryType::TEXT)
			{
				pShaderSystem->SetShaderUniform(hCurrentlyBoundShader, m_DebugTextShaderUniforms.color, 0, pCurGeo.iInstanceID, &pCurGeo.color);
				pShaderSystem->SetShaderUniform(hCurrentlyBoundShader, m_DebugTextShaderUniforms.model, 0, pCurGeo.iInstanceID, &pCurGeo.geometryInstance.matLocalToWorldTransform);

				pShaderSystem->BindPerInstanceUniforms(pCurGeo.iInstanceID);
				m_Config.pRenderer->DrawGeometry(pCurGeo.geometryInstance, &m_DebugVertexBuffer, &m_DebugIndexBuffer);

				pCurGeo.geometryInstance.matLocalToWorldTransform = matSave;
			}
			else
			{
				pShaderSystem->SetShaderUniform(hCurrentlyBoundShader, m_DebugLineShaderUniforms.color, 0, pCurGeo.iInstanceID, &pCurGeo.color);
				pShaderSystem->SetShaderUniform(hCurrentlyBoundShader, m_DebugLineShaderUniforms.model, 0, pCurGeo.iInstanceID, &pCurGeo.geometryInstance.matLocalToWorldTransform);

				pShaderSystem->BindPerInstanceUniforms(pCurGeo.iInstanceID);
				m_Config.pRenderer->DrawGeometry(pCurGeo.geometryInstance, &m_DebugVertexBuffer, &m_DebugIndexBuffer);

			}
		}

		m_Config.pRenderer->EndRendererView();
	}
	else
	{
		LOG_ERROR("Failed to begin Debug Renderpass. Skipping debug rendering.");
		return;
	}*/
}

//-------------------------------------------------------------------------
// Purpose: Adds a point (provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddPoint(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::LINE;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugPointInstance");

	// Create a 3D cross at the point that extends out from the origin by the given radius.
	Vector ptVerts[4]{};
	ptVerts[1].x += flRadius;
	ptVerts[2].y += flRadius;
	ptVerts[3].z += flRadius;

	uint32_t indices[6] = { 0, 1, 0, 2, 0, 3 };

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, &m_DebugIndexBuffer, 4, sizeof(Vector3D), ptVerts, 6, indices, pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this geometry will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a line (endpoints provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::LINE;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugLineInstance");

	Vector3D vecPositions[2] = { ptStartPos, ptEndPos };
	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, 2, sizeof(Vector3D), vecPositions, 0, nullptr, pNewGeometry.geometryInstance);

	// Just use an identity matrix for the transform (this shouldn't ever move around).
	pNewGeometry.geometryInstance.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a bounding box of a piece of geometry to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::LINE;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugBBoxInstance");

	Vector3D& vecLocalCenter = pGeometry.vecCenter;
	Vector3D& vecLocalMins = pGeometry.vecMinExtents;
	Vector3D& vecLocalMaxs = pGeometry.vecMaxExtents;

	const int iNumVertices = 24;
	Vector3D ptVertices[iNumVertices]{};

	// This first set of verts/edges form the horizontal square along the bottom of the geometry.
	ptVertices[0] = vecLocalMins;
	ptVertices[1] = vecLocalMins;
	ptVertices[1].z = vecLocalMaxs.z;

	ptVertices[2] = vecLocalMins;
	ptVertices[2].x = vecLocalMaxs.x;
	ptVertices[3] = vecLocalMins;
	ptVertices[3].x = vecLocalMaxs.x;
	ptVertices[3].z = vecLocalMaxs.z;

	ptVertices[4] = vecLocalMins;
	ptVertices[5] = vecLocalMins;
	ptVertices[5].x = vecLocalMaxs.x;

	ptVertices[6] = vecLocalMins;
	ptVertices[6].z = vecLocalMaxs.z;
	ptVertices[7] = vecLocalMins;
	ptVertices[7].x = vecLocalMaxs.x;
	ptVertices[7].z = vecLocalMaxs.z;

	// Directly copy the verts and raise them all up to max Y to form the horizontal square along the top of the geometry.
	Platform::MemoryCopy(&ptVertices[8], &ptVertices[0], sizeof(Vector3D) * 8);
	for (int idx = 8; idx < 16; idx++)
	{
		ptVertices[idx].y = vecLocalMaxs.y;
	}

	// Now for the four corners of the box.
	ptVertices[16] = vecLocalMins;
	ptVertices[17] = vecLocalMins;
	ptVertices[17].y = vecLocalMaxs.y;

	ptVertices[18] = vecLocalMins;
	ptVertices[18].x = vecLocalMaxs.x;
	ptVertices[19] = vecLocalMins;
	ptVertices[19].x = vecLocalMaxs.x;
	ptVertices[19].y = vecLocalMaxs.y;

	ptVertices[20] = vecLocalMins;
	ptVertices[20].z = vecLocalMaxs.z;
	ptVertices[21] = vecLocalMins;
	ptVertices[21].z = vecLocalMaxs.z;
	ptVertices[21].y = vecLocalMaxs.y;

	ptVertices[22] = vecLocalMaxs;
	ptVertices[22].y = vecLocalMins.y;
	ptVertices[23] = vecLocalMaxs;

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);

	// Store off the transform index.
	//pNewGeometry.geometryInstance.iLocalToWorldTransform = pGeometry.iLocalToWorldTransform;
	pNewGeometry.geometryInstance.matLocalToWorldTransform = pGeometry.matLocalToWorldTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a bounding box of a piece of geometry to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddBBox(Vector vecMins, Vector vecMaxs, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::LINE;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugBBoxInstance");

	const int iNumVertices = 24;
	Vector3D ptVertices[iNumVertices]{};

	// This first set of verts/edges form the horizontal square along the bottom of the geometry.
	ptVertices[0] = vecMins;
	ptVertices[1] = vecMins;
	ptVertices[1].z = vecMaxs.z;

	ptVertices[2] = vecMins;
	ptVertices[2].x = vecMaxs.x;
	ptVertices[3] = vecMins;
	ptVertices[3].x = vecMaxs.x;
	ptVertices[3].z = vecMaxs.z;

	ptVertices[4] = vecMins;
	ptVertices[5] = vecMins;
	ptVertices[5].x = vecMaxs.x;

	ptVertices[6] = vecMins;
	ptVertices[6].z = vecMaxs.z;
	ptVertices[7] = vecMins;
	ptVertices[7].x = vecMaxs.x;
	ptVertices[7].z = vecMaxs.z;

	// Directly copy the verts and raise them all up to max Y to form the horizontal square along the top of the geometry.
	Platform::MemoryCopy(&ptVertices[8], &ptVertices[0], sizeof(Vector3D) * 8);
	for (int idx = 8; idx < 16; idx++)
	{
		ptVertices[idx].y = vecMaxs.y;
	}

	// Now for the four corners of the box.
	ptVertices[16] = vecMins;
	ptVertices[17] = vecMins;
	ptVertices[17].y = vecMaxs.y;

	ptVertices[18] = vecMins;
	ptVertices[18].x = vecMaxs.x;
	ptVertices[19] = vecMins;
	ptVertices[19].x = vecMaxs.x;
	ptVertices[19].y = vecMaxs.y;

	ptVertices[20] = vecMins;
	ptVertices[20].z = vecMaxs.z;
	ptVertices[21] = vecMins;
	ptVertices[21].z = vecMaxs.z;
	ptVertices[21].y = vecMaxs.y;

	ptVertices[22] = vecMaxs;
	ptVertices[22].y = vecMins.y;
	ptVertices[23] = vecMaxs;

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);

	pNewGeometry.geometryInstance.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a sphere (origin provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddSphere(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::LINE;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	pNewGeometry.color = color;

	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugTest_Sphere");

	const int iNumVertices = 72;
	Vector3D ptVertices[iNumVertices]{};

	const float fl2PI = 2.0f * kPI_F;
	int iCurrentVert = 0;

	for (float flAngle = (kPI_F / 12.0f); flAngle < fl2PI; (flAngle += kPI_F / 12.0f))
	{
		ptVertices[iCurrentVert].x = flRadius * M_COS_F(flAngle);
		ptVertices[iCurrentVert].y = flRadius * M_SIN_F(flAngle);
		iCurrentVert += 1;
	}

	for (float flAngle = (kPI_F / 12.0f); flAngle < fl2PI; (flAngle += kPI_F / 12.0f))
	{
		ptVertices[iCurrentVert].x = flRadius * M_COS_F(flAngle);
		ptVertices[iCurrentVert].z = flRadius * M_SIN_F(flAngle);
		iCurrentVert += 1;
	}

	for (float flAngle = (kPI_F / 12.0f); flAngle < fl2PI; (flAngle += kPI_F / 12.0f))
	{
		ptVertices[iCurrentVert].y = flRadius * M_COS_F(flAngle);
		ptVertices[iCurrentVert].z = flRadius * M_SIN_F(flAngle);
		iCurrentVert += 1;
	}

	const int iNumIndices = 144;
	uint32_t indices[iNumIndices]{};
	iCurrentVert = 1;
	// Setup index data.
	for (int iIndex = 0; iIndex < iNumIndices; iIndex += 2)
	{
		indices[iIndex] = iCurrentVert - 1;
		indices[iIndex + 1] = iCurrentVert;

		iCurrentVert++;
	}

	// Make sure the last vert of the circle links back to the first vert (because it's a circle!)
	indices[47] = indices[0];
	indices[95] = indices[48];
	indices[143] = indices[96];

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, &m_DebugIndexBuffer, iNumVertices, sizeof(Vector3D), ptVertices, iNumIndices, indices, pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this sphere will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a piece of text to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddText(IFontSystem* pFontSystem, Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, float flDuration)
{
	// TODO: Make the text center-justified (i.e. the 'origin' of the text is at the center of the geometry).
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::TEXT;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	GeometryConfig_t textConfig = pFontSystem->Acquire3DConfigForText(hFont, 12.0f, pszText);

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, &m_DebugIndexBuffer, textConfig, pNewGeometry.geometryInstance);

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugTextShader, &pNewGeometry.iInstanceID, "DebugTextInstance");

	// Apply the font atlas.
	TextureHandle_t hTexture = pFontSystem->AcquireFontAtlasTexture(hFont);
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugTextShaderUniforms.fontAtlas, 0, pNewGeometry.iInstanceID, &hTexture);

	// Create a new transform matrix.
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a camera frustum to the debug geometries.
//-------------------------------------------------------------------------
int CDebugRenderer::AddCameraFrustum(Camera_t& pCamera, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.eType = DebugGeometryType::CAMERA_FRUSTUM;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "CameraFrustum");

	const int iNumVertices = 24;
	Vector3D ptVertices[iNumVertices]{};

	Vector vecCameraPosition = pCamera.vecPosition;

	ptVertices[0] = vecCameraPosition + Vector3D{ -50.0f, 0.0f, -60.0f, };
	ptVertices[1] = vecCameraPosition + Vector3D{ 50.0f, 0.0f, -60.0f };
	{
		// Define the frustum vertices in clip-space.
		// Then, during the rendering loop, we'll project into worldspace.
		Vector frustumVerts[8]{};
		frustumVerts[0] = Vector{ 1.0f, 1.0f, 1.0f };
		frustumVerts[1] = Vector{ -1.0f, 1.0f, 1.0f };
		frustumVerts[2] = Vector{ 1.0f, -1.0f, 1.0f };
		frustumVerts[3] = Vector{ -1.0f, -1.0f, 1.0f };
		frustumVerts[4] = Vector{ 1.0f, 1.0f, -1.0f };
		frustumVerts[5] = Vector{ -1.0f, 1.0f, -1.0f };
		frustumVerts[6] = Vector{ 1.0f, -1.0f, -1.0f };
		frustumVerts[7] = Vector{ -1.0f, -1.0f, -1.0f };

		// Defines the far-clipping plane.
		ptVertices[0] = frustumVerts[0];
		ptVertices[1] = frustumVerts[1];
		ptVertices[2] = frustumVerts[2];
		ptVertices[3] = frustumVerts[3];
		ptVertices[4] = frustumVerts[0];
		ptVertices[5] = frustumVerts[2];
		ptVertices[6] = frustumVerts[1];
		ptVertices[7] = frustumVerts[3];

		// Defines the near-clipping plane.
		ptVertices[8] = frustumVerts[4];
		ptVertices[9] = frustumVerts[5];
		ptVertices[10] = frustumVerts[6];
		ptVertices[11] = frustumVerts[7];
		ptVertices[12] = frustumVerts[4];
		ptVertices[13] = frustumVerts[6];
		ptVertices[14] = frustumVerts[5];
		ptVertices[15] = frustumVerts[7];

		// Connect the four corners.
		ptVertices[16] = frustumVerts[0];
		ptVertices[17] = frustumVerts[4];
		ptVertices[18] = frustumVerts[1];
		ptVertices[19] = frustumVerts[5];
		ptVertices[20] = frustumVerts[2];
		ptVertices[21] = frustumVerts[6];
		ptVertices[22] = frustumVerts[3];
		ptVertices[23] = frustumVerts[7];
	}

	// Set a good ol' identity matrix here, we'll need to hook this to the camera at somepoint.
	Matrix4x4 matTransform = Matrix4x4{ 1.0f };
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	// TODO: Fix this
	//m_Config.pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Releases all the data related to a debug geometry instance.
// Input:   debugGeoHandle - A handle to the debug geometry to release.
// Output:  none.
//-------------------------------------------------------------------------
void CDebugRenderer::RemoveDebugGeometry(int debugGeoHandle)
{
	DebugGeometry_t& pGeoToRemove = m_GeometryData[debugGeoHandle];
	CAssert(pGeoToRemove.iInstanceID != kInvalidID_U32);

	m_Config.pShaderSystem->ReleaseShaderInstanceResources(pGeoToRemove.eType == DebugGeometryType::TEXT ? m_hDebugTextShader : m_hDebugLineShader, pGeoToRemove.iInstanceID);
	
	// TODO: Fix this
	//m_Config.pRenderer->ReleaseGeometryResources(m_DebugVertexBuffer, m_DebugIndexBuffer, pGeoToRemove.geometryInstance);

	pGeoToRemove.iInstanceID = kInvalidID_U32;
	pGeoToRemove.eType = DebugGeometryType::UNDEFINED;
}

//-------------------------------------------------------------------------
// Purpose: Gets the first free slot for a new piece of rendering geometry.
// Input:   none.
// Output:  The new index, if one was found, otherwise an invalid ID.
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

	LOG_ERROR("Failed to find a free slot for new debug geometry data. Increase configuration to allow for more.");
	return kInvalidID_U32;
}
