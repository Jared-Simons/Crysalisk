//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#include "DebugRenderer.h"

#include "Camera.h"
#include <Containers/CVector.h>
#include <Math/MathLib.h>
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

	virtual int		AddPoint(Vector& pOrigin, const float flRadius, Vector4D color) override;
	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color);
	virtual int		AddBBox(GeometryInstance_t& pGeometry, Vector4D color) override;
	virtual int		AddSphere(Vector& pOrigin, const float flRadius, Vector4D color) override;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color) override;

private:
	int				GetFirstFreeGeometryDataIndex() const;

private:
	RendererView	m_DebugView{};

	RenderBuffer		m_DebugVertexBuffer{};
	RenderBuffer		m_DebugIndexBuffer{};

	ShaderHandle_t				m_hDebugLineShader{ kInvalidID_U32 };
	DebugShaderUniformLocations	m_DebugShaderUniforms{};

	int					m_iNumGeometryInstances{ 0 };
	DebugGeometry_t*	m_GeometryData{ nullptr };

	// FRUSTUM TEST
	DebugGeometry_t		m_TestCameraFrustum{};
	Vector3D			m_vecCameraPosition{};
	Vector3D			m_CameraEulerAngles{};
	Vector3D			m_vecCameraForward{};
	// END FRUSTUM TEST
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

	if (!g_pRenderer->CreateRenderBuffer(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Vertex_Buffer", m_DebugVertexBuffer))
	{
		LOG_ERROR("Failed to create debug vertex buffer!");
		return false;
	}

	if (!g_pRenderer->CreateRenderBuffer(sizeof(uint32_t) * 1024, eRenderBufferUsage::INDEX, true, "Debug_Renderer_Index_Buffer", m_DebugIndexBuffer))
	{
		LOG_ERROR("Failed to create debug index buffer!");
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
	g_pRenderer->DestroyRenderBuffer(m_DebugIndexBuffer);
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
			if (m_GeometryData[1].iInstanceID != kInvalidID_U32)
			{
				DebugGeometry_t& pTestGeometry = m_GeometryData[1];
				g_pShaderSystem->BindShader(m_hDebugLineShader);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.projection, 0, 0, &pFrameData.matProjection);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.view, 0, 0, &pFrameData.matView);
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.color, 0, pTestGeometry.iInstanceID, &pTestGeometry.color);

				// DEBUGTODO: Get this from somewhere!
				Matrix4x4 matModel = pFrameData.pTransformMatrices[pTestGeometry.geometryInstance.iLocalToWorldTransform];
				matModel = pTestGeometry.geometryInstance.matLocalToWorldTransform;
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.model, 0, pTestGeometry.iInstanceID, &matModel);

				g_pShaderSystem->BindPerInstanceUniforms(pTestGeometry.iInstanceID);
				g_pRenderer->DrawGeometry(pTestGeometry.geometryInstance, &m_DebugVertexBuffer, &m_DebugIndexBuffer);
			}

			if (m_TestCameraFrustum.iInstanceID != kInvalidID_U32)
			{
				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.color, 0, m_TestCameraFrustum.iInstanceID, &m_TestCameraFrustum.color);

				// Generate a matrix that follows the camera's transform.
				Matrix4x4 matModel{ 1.0f };
				matModel = pFrameData.matProjection.Inverse();
				matModel = matModel * pFrameData.matView.Inverse();

				g_pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugShaderUniforms.model, 0, m_TestCameraFrustum.iInstanceID, &matModel);

				g_pShaderSystem->BindPerInstanceUniforms(m_TestCameraFrustum.iInstanceID);
				g_pRenderer->DrawGeometry(m_TestCameraFrustum.geometryInstance, &m_DebugVertexBuffer, nullptr);
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
int CDebugRenderer::AddPoint(Vector& pOrigin, const float flRadius, Vector4D color)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;

	// Acquire new shader instance resources.
	g_pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugPointInstance");

	// Create a 3D cross at the point that extends out from the origin by the given radius.
	Vector ptVerts[4] = { pOrigin, pOrigin, pOrigin, pOrigin };
	ptVerts[1].x += flRadius;
	ptVerts[2].y += flRadius;
	ptVerts[3].z += flRadius;

	uint32_t indices[6] = { 0, 1, 0, 2, 0, 3 };

	g_pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, &m_DebugIndexBuffer, 4, sizeof(Vector3D), ptVerts, 6, indices, pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this geometry will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	return iDebugGeometryIndex;
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
int CDebugRenderer::AddBBox(GeometryInstance_t& pGeometry, Vector4D color)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;

	// Acquire new shader instance resources.
	g_pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugBBoxInstance");

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

	g_pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);

	// Store off the transform index.
	pNewGeometry.geometryInstance.iLocalToWorldTransform = pGeometry.iLocalToWorldTransform;
	pNewGeometry.geometryInstance.matLocalToWorldTransform = pGeometry.matLocalToWorldTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
int CDebugRenderer::AddSphere(Vector& pOrigin, const float flRadius, Vector4D color)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];

	pNewGeometry.color = color;

	g_pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "DebugTest_Sphere");

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

	// Create make sure the last vert of the circle links back to the first vert (because it's a circle!)
	indices[47] = indices[0];
	indices[95] = indices[48];
	indices[143] = indices[96];

	g_pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, &m_DebugIndexBuffer, iNumVertices, sizeof(Vector3D), ptVertices, iNumIndices, indices, pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this sphere will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
int CDebugRenderer::AddCameraFrustum(Camera_t& pCamera, Vector4D color)
{
	//int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	//DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	DebugGeometry_t& pNewGeometry = m_TestCameraFrustum;
	pNewGeometry.color = color;
	bool bAcquireNewResources = pNewGeometry.iInstanceID == kInvalidID_U32;

	if (bAcquireNewResources)
	{
		// Acquire new shader instance resources.
		g_pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.iInstanceID, "CameraFrustum");
	}

	const int iNumVertices = 24;
	Vector3D ptVertices[iNumVertices]{};

	ptVertices[0] = m_vecCameraPosition + Vector3D{ -50.0f, 0.0f, -60.0f, };
	ptVertices[1] = m_vecCameraPosition + Vector3D{ 50.0f, 0.0f, -60.0f };
	// DEBUGTODO: Reuse from the scene's frustum instead of generating a second one.
	Frustum cameraFrustum = pCamera.CreateFrustumFromCamera();
	{
		// DEBUGTODO: Document this process.
		// DEBUGTODO: This frustum is compressed slightly so it actually shows up on the screen (we don't have multiple cameras working yet).
		//				Make sure to make this go from (-1,-1,-1) to (1,1,1).

		Vector frustumVerts[8]{};
		frustumVerts[0] = Vector{ 0.95f, 0.95f, 0.95f };
		frustumVerts[1] = Vector{ -0.95f, 0.95f, 0.95f };
		frustumVerts[2] = Vector{ 0.95f, -0.95f, 0.95f };
		frustumVerts[3] = Vector{ -0.95f, -0.95f, 0.95f };
		frustumVerts[4] = Vector{ 0.95f, 0.95f, -0.9f };
		frustumVerts[5] = Vector{ -0.95f, 0.95f, -0.9f };
		frustumVerts[6] = Vector{ 0.95f, -0.95f, -0.9f };
		frustumVerts[7] = Vector{ -0.95f, -0.95f, -0.9f };

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

	if (bAcquireNewResources)
	{
		g_pRenderer->AcquireGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);
	}
	else
	{
		//g_pRenderer->UpdateGeometryResources(&m_DebugVertexBuffer, nullptr, iNumVertices, sizeof(Vector3D), ptVertices, 0, nullptr, pNewGeometry.geometryInstance);
	}

	// Save off details about the camera.
	m_vecCameraPosition = pCamera.vecPosition;
	// DEBUGLEFTOFF: Figure this out!
	m_CameraEulerAngles = Vector3D{ pCamera.Yaw, pCamera.Pitch, 0.0f };
	m_vecCameraForward = pCamera.vecFront;

	return 0;
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
