//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#include "DebugGeometrySystem.h"

#include "Core/Engine.h"
#include <Containers/CVector.h>
#include <Math/MathLib.h>
#include "Renderer/Renderer.h"
#include <Renderer/Camera.h>
#include <Strings/String_Utils.h>
#include "Systems/FontSystem.h"
#include "Systems/ShaderSystem.h"

//-------------------------------------------------------------------------
// Purpose: An instance of debug geometry, contains all of the
//			data/resources necessary to render debug geometry.
//-------------------------------------------------------------------------
struct DebugGeometry_t
{
	Vector4D			color{};
	GeometryInstance_t	geometryInstance{};
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
class CDebugGeometrySystem : public IDebugGeometrySystem
{
public:
	virtual bool	Initialize(DebugRendererConfig_t& pAppConfig) override;
	virtual bool	InitializeDebugShaders() override;
	virtual void	Shutdown() override;

	virtual void	Update() override;
	virtual void	PopulateRenderPacket(Utl::CVector<GeometryInstance_t*>& pGeometries, RenderBuffer** pOutVertexBuffer, RenderBuffer** pOutIndexBuffer) override;

	virtual int		AddPoint(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) override;
	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, float flDuration) override;
	virtual int		AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration) override;
	virtual int		AddSphere(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) override;
	virtual int		AddText(class IFontSystem* pFontSystem, Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, float flDuration) override;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color, float flDuration) override;
	virtual void	RemoveDebugGeometry(int debugGeoHandle) override;

	virtual void	UpdateDebugGeometryTransform(uint32_t debugGeoHandle, Matrix4x4 matTransform) override;

private:
	int				GetFirstFreeGeometryDataIndex() const;

private:
	DebugRendererConfig_t		m_Config{};

	RenderBuffer*				m_pDebugVertexBuffer{};
	RenderBuffer*				m_pDebugIndexBuffer{};

	ShaderHandle_t					m_hDebugLineShader{ kInvalidID_U32 };
	DebugLineShaderUniformLocations	m_DebugLineShaderUniforms{};

	ShaderHandle_t					m_hDebugTextShader{ kInvalidID_U32 };
	DebugTextShaderUniformLocations	m_DebugTextShaderUniforms{};

	unsigned int		m_iActiveGeometryInstances{ 0 };
	DebugGeometry_t*	m_GeometryData{ nullptr };

	// NOTE: This gets updated from the main renderer's clock.
	float			m_flRendererTime{ 0.0f };
};

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IDebugGeometrySystem::AllocateDebugGeometrySystemState(IDebugGeometrySystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold debug render system state was not provided! Debug render system creation failed!");
		return;
	}

	*ppSystemState = new CDebugGeometrySystem;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CDebugGeometrySystem::Initialize(DebugRendererConfig_t& pConfig)
{
	// Save off config.
	m_Config = pConfig;

	// Setup debug geometry array.
	m_GeometryData = new DebugGeometry_t[pConfig.uMaxDebugGeometries];

	// Set up geometry buffers.
	m_pDebugVertexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Line_Vertex_Buffer");
	if (m_pDebugVertexBuffer == nullptr)
	{
		LOG_ERROR("Failed to create debug vertex buffer!");
		return false;
	}

	m_pDebugIndexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(uint32_t) * 1024, eRenderBufferUsage::INDEX, true, "Debug_Renderer_Line_Index_Buffer");
	if (m_pDebugIndexBuffer == nullptr)
	{
		LOG_ERROR("Failed to create debug index buffer!");
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Perform some post-initialization setup.
//-------------------------------------------------------------------------
bool CDebugGeometrySystem::InitializeDebugShaders()
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
void CDebugGeometrySystem::Shutdown()
{
	// Release debug geometry data.
	delete[] m_GeometryData;
	m_GeometryData = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Updates internal debug renderer state.
//-------------------------------------------------------------------------
void CDebugGeometrySystem::Update()
{
	m_flRendererTime = m_Config.pRenderer->GetRendererTime();
	uint32_t numGeoFound = 0;
	for (uint32_t iGeo = 0; numGeoFound < m_iActiveGeometryInstances && iGeo < m_Config.uMaxDebugGeometries; iGeo++)
	{
		DebugGeometry_t& pCurGeo = m_GeometryData[iGeo];
		if (pCurGeo.geometryInstance.hShaderInstance == kInvalidID_U32)
			continue;

		if (pCurGeo.flTimeToRemove < 0.0f)
			continue;

		if (pCurGeo.flTimeToRemove < m_flRendererTime)
		{
			// Time to remove this geo.
			RemoveDebugGeometry(iGeo);
			numGeoFound++;
		}
	}
}

void CDebugGeometrySystem::PopulateRenderPacket(Utl::CVector<GeometryInstance_t*>& pGeometries, RenderBuffer** pOutVertexBuffer, RenderBuffer** pOutIndexBuffer)
{
	pGeometries.SetCount(IntMin(m_iActiveGeometryInstances, m_Config.uMaxDebugGeometries));

	*pOutVertexBuffer = m_pDebugVertexBuffer;
	*pOutIndexBuffer = m_pDebugIndexBuffer;

	uint32_t numGeoFound = 0;
	for (uint32_t iGeo = 0; iGeo < m_Config.uMaxDebugGeometries && numGeoFound < m_iActiveGeometryInstances; iGeo++)
	{
		if (m_GeometryData[iGeo].geometryInstance.hShaderInstance != kInvalidID_U32)
		{
			pGeometries[numGeoFound] = &m_GeometryData[iGeo].geometryInstance;
			numGeoFound++;
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Adds a point (provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddPoint(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugLineShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.geometryInstance.hShaderInstance, "DebugPointInstance");
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);

	// Create a 3D cross at the point that extends out from the origin by the given radius.
	Vector ptVerts[4]{};
	ptVerts[1].x += flRadius;
	ptVerts[2].y += flRadius;
	ptVerts[3].z += flRadius;

	uint32_t indices[6] = { 0, 1, 0, 2, 0, 3 };

	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, 4, sizeof(Vector), 0, ptVerts, 6, indices, &pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this geometry will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a line (endpoints provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugLineShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.geometryInstance.hShaderInstance, "DebugLineInstance");
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);

	Vector3D vecPositions[2] = { ptStartPos, ptEndPos };
	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, 4, sizeof(Vector), 0, vecPositions, 0, nullptr, &pNewGeometry.geometryInstance);

	// Just use an identity matrix for the transform (this shouldn't ever move around).
	pNewGeometry.geometryInstance.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a bounding box of a piece of geometry to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugLineShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.geometryInstance.hShaderInstance, "DebugBBoxInstance");
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);

	Vector3D& vecLocalCenter = pGeometry.vecCenter;
	Vector3D& vecLocalMins = pGeometry.vecMinExtents;
	Vector3D& vecLocalMaxs = pGeometry.vecMaxExtents;

	const int iNumVertices = 8;
	Vector ptVertices[iNumVertices]{};

	const int iNumIndices = 24;
	uint32_t indices[iNumIndices] = { 0, 1, 2, 3 };

	// This first set of verts/edges form the horizontal square along the bottom of the geometry.
	ptVertices[0] = vecLocalMins;
	ptVertices[1] = vecLocalMins;
	ptVertices[1].z = vecLocalMaxs.z;

	ptVertices[2] = vecLocalMins;
	ptVertices[2].x = vecLocalMaxs.x;
	ptVertices[3] = vecLocalMins;
	ptVertices[3].x = vecLocalMaxs.x;
	ptVertices[3].z = vecLocalMaxs.z;

	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 3;
	indices[4] = 0;
	indices[5] = 2;
	indices[6] = 1;
	indices[7] = 3;

	// Directly copy the verts and raise them all up to the max Y to form the horizontal square along the top of the geometry.
	for (int idx = 4; idx < 8; idx++)
	{
		ptVertices[idx] = ptVertices[idx - 4];
		ptVertices[idx].y = vecLocalMaxs.y;
	}

	for (int idx = 8; idx < 16; idx++)
	{
		indices[idx] = indices[idx - 8] + 4;
	}

	// Now for the four corners of the box.
	for (int idx = 16, i = 0; idx < iNumIndices; idx += 2, i++)
	{
		indices[idx + 0] = i;
		indices[idx + 1] = i + 4;
	}

	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, iNumVertices, sizeof(Vector), 0, ptVertices, iNumIndices, indices, &pNewGeometry.geometryInstance);

	// Store off the transform index.
	pNewGeometry.geometryInstance.matLocalToWorldTransform = pGeometry.matLocalToWorldTransform;
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a sphere (origin provided in worldspace) to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddSphere(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugLineShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	pNewGeometry.color = color;

	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.geometryInstance.hShaderInstance, "DebugTest_Sphere");
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);

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

	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, iNumVertices, sizeof(Vector), 0, ptVertices, iNumIndices, indices, &pNewGeometry.geometryInstance);

	// Create a new transform matrix (this method assumes that this sphere will not be parented with another transform).
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a piece of text to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddText(IFontSystem* pFontSystem, Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, float flDuration)
{
	// TODO: Make the text center-justified (i.e. the 'origin' of the text is at the center of the geometry).
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugTextShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	GeometryConfig_t textConfig = pFontSystem->Acquire3DConfigForText(hFont, 12.0f, pszText);

	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, 
												 textConfig.Vertices.Count(), textConfig.Vertices.Stride(), 0, textConfig.Vertices.Data(),
												 textConfig.Indices.Count(), textConfig.Indices.Data(), &pNewGeometry.geometryInstance);

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugTextShader, &pNewGeometry.geometryInstance.hShaderInstance, "DebugTextInstance");

	// Apply the font atlas.
	TextureHandle_t hTexture = pFontSystem->AcquireFontAtlasTexture(hFont);
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugTextShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugTextShader, m_DebugTextShaderUniforms.fontAtlas, 0, pNewGeometry.geometryInstance.hShaderInstance, &hTexture);

	// Create a new transform matrix.
	Matrix4x4 matTransform{ 1.0f };
	matTransform = Matrix4x4::Translate(matTransform, pOrigin);
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matTransform;
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Adds a camera frustum to the debug geometries.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::AddCameraFrustum(Camera_t& pCamera, Vector4D color, float flDuration)
{
	int iDebugGeometryIndex = GetFirstFreeGeometryDataIndex();
	if (iDebugGeometryIndex == kInvalidID_U32)
		return kInvalidID_U32;

	DebugGeometry_t& pNewGeometry = m_GeometryData[iDebugGeometryIndex];
	pNewGeometry.color = color;
	pNewGeometry.flTimeToRemove = m_flRendererTime + flDuration;
	pNewGeometry.geometryInstance.hShader = m_hDebugLineShader;

	if (flDuration < 0.0f)
	{
		// A negative duration means 'never remove this geometry'.
		pNewGeometry.flTimeToRemove = -1.0f;
	}

	// Acquire new shader instance resources.
	m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDebugLineShader, &pNewGeometry.geometryInstance.hShaderInstance, "CameraFrustum");
	m_Config.pShaderSystem->SetShaderUniform(m_hDebugLineShader, m_DebugLineShaderUniforms.color, 0, pNewGeometry.geometryInstance.hShaderInstance, &color);

	// Define the frustum vertices in clip-space.
	// Then unproject into world-space.
	const int iNumVertices = 8;
	Vector ptVertices[iNumVertices]{};

	const int iNumIndices = 24;
	uint32_t indices[iNumIndices];

	Vector vecCameraPosition = pCamera.vecPosition;

	ptVertices[0] = Vector{ 1.0f, 1.0f, 1.0f };
	ptVertices[1] = Vector{ -1.0f, 1.0f, 1.0f };
	ptVertices[2] = Vector{ 1.0f, -1.0f, 1.0f };
	ptVertices[3] = Vector{ -1.0f, -1.0f, 1.0f };
	ptVertices[4] = Vector{ 1.0f, 1.0f, -1.0f };
	ptVertices[5] = Vector{ -1.0f, 1.0f, -1.0f };
	ptVertices[6] = Vector{ 1.0f, -1.0f, -1.0f };
	ptVertices[7] = Vector{ -1.0f, -1.0f, -1.0f };

	// Defines the far-clipping plane.
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 3;
	indices[4] = 0;
	indices[5] = 2;
	indices[6] = 1;
	indices[7] = 3;

	// Defines the near-clipping plane.
	indices[8] = 4;
	indices[9] = 5;
	indices[10] = 6;
	indices[11] = 7;
	indices[12] = 4;
	indices[13] = 6;
	indices[14] = 5;
	indices[15] = 7;

	// Connect the four corners.
	indices[16] = 0;
	indices[17] = 4;
	indices[18] = 1;
	indices[19] = 5;
	indices[20] = 2;
	indices[21] = 6;
	indices[22] = 3;
	indices[23] = 7;

	// Clip->World
	Matrix4x4 matClipToWorld = Matrix4x4(1.0f);
	matClipToWorld = matClipToWorld * g_pEngine->GetEngineWindow()->GetViewport().matProjection.Inverse();
	matClipToWorld = matClipToWorld * pCamera.GetViewMatrix().Inverse();
	pNewGeometry.geometryInstance.matLocalToWorldTransform = matClipToWorld;

	m_Config.pRenderer->AcquireGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, iNumVertices, sizeof(Vector), 0, ptVertices, iNumIndices, indices, &pNewGeometry.geometryInstance);
	m_iActiveGeometryInstances++;

	return iDebugGeometryIndex;
}

//-------------------------------------------------------------------------
// Purpose: Releases all the data related to a debug geometry instance.
// Input:   debugGeoHandle - A handle to the debug geometry to release.
// Output:  none.
//-------------------------------------------------------------------------
void CDebugGeometrySystem::RemoveDebugGeometry(int debugGeoHandle)
{
	DebugGeometry_t& pGeoToRemove = m_GeometryData[debugGeoHandle];
	CAssert(pGeoToRemove.geometryInstance.hShaderInstance != kInvalidID_U32);

	m_Config.pShaderSystem->ReleaseShaderInstanceResources(pGeoToRemove.geometryInstance.hShader, pGeoToRemove.geometryInstance.hShaderInstance);
	m_Config.pRenderer->ReleaseGeometryResources(*m_pDebugVertexBuffer, *m_pDebugIndexBuffer, pGeoToRemove.geometryInstance);

	pGeoToRemove.geometryInstance.hShaderInstance = kInvalidID_U32;
	m_iActiveGeometryInstances--;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation.
//-------------------------------------------------------------------------
void CDebugGeometrySystem::UpdateDebugGeometryTransform(uint32_t debugGeoHandle, Matrix4x4 matTransform)
{
	if (debugGeoHandle == kInvalidID_U32)
		return;

	m_GeometryData[debugGeoHandle].geometryInstance.matLocalToWorldTransform = matTransform;
}

//-------------------------------------------------------------------------
// Purpose: Gets the first free slot for a new piece of rendering geometry.
// Input:   none.
// Output:  The new index, if one was found, otherwise an invalid ID.
//-------------------------------------------------------------------------
int CDebugGeometrySystem::GetFirstFreeGeometryDataIndex() const
{
	for (unsigned int idx = 0; idx < m_Config.uMaxDebugGeometries; idx++)
	{
		if (m_GeometryData[idx].geometryInstance.hShaderInstance == kInvalidID_U32)
		{
			// Found an unused index!
			return idx;
		}
	}

	LOG_ERROR("Failed to find a free slot for new debug geometry data. Increase configuration to allow for more.");
	return kInvalidID_U32;
}
