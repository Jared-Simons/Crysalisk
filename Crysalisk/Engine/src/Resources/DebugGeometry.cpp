//=============================================================================
// Author: Jared Simons
// Date Created: 5/21/2026
// 
// Purpose: Debug Geometry Generation Tool
//=============================================================================
#include "DebugGeometry.h"

#include "Core/Engine.h"
#include "Renderer/Renderer.h"
#include "Systems/ShaderSystem.h"

namespace DebugRender
{
	struct DebugShaderData_t
	{
		IRenderer*		pRenderer{ nullptr };
		IShaderSystem*	pShaderSystem{ nullptr };

		ShaderHandle_t	hDebugLineShader{ kInvalidID_U32 };
		uint16_t		iDebugLineIdx_Projection{ kInvalidID_U16 };
		uint16_t		iDebugLineIdx_View{ kInvalidID_U16 };
		uint16_t		iDebugLineIdx_Color{ kInvalidID_U16 };
		uint16_t		iDebugLineIdx_Model{ kInvalidID_U16 };

		bool			bDebugShaderDataInitialized{ false };
	};

	static DebugShaderData_t s_DebugShaderData{};
	static void InitializeDebugShaderData();

	DebugGeometry_t	AddPoint(Vector ptOrigin, const float flRadius, Vector4D color, float flDuration)
	{
		// Initialize the local shader data if needed.
		if (!s_DebugShaderData.bDebugShaderDataInitialized)
		{
			InitializeDebugShaderData();
		}

		DebugGeometry_t newGeo{};
		newGeo.color = color;
		newGeo.flTimeToRemove = s_DebugShaderData.pRenderer->GetRendererTime() + flDuration;
		
		if (flDuration < 0.0f)
		{
			// A negative duration means 'never remove this geometry'.
			newGeo.flTimeToRemove = -1.0f;
		}

		// Acquire new shader instance resources.
		ShaderHandle_t hDebugShader = s_DebugShaderData.hDebugLineShader;
		s_DebugShaderData.pShaderSystem->AcquireShaderInstanceResources(hDebugShader, &newGeo.iInstanceID, "DebugPointInstance");

		// Create a 3D cross at the point that extends out from the origin by the given radius.
		Vector ptVerts[4]{};
		ptVerts[1].x += flRadius;
		ptVerts[2].y += flRadius;
		ptVerts[3].z += flRadius;

		uint32_t indices[6] = { 0, 1, 0, 2, 0, 3 };

		IRenderer* pRenderer = s_DebugShaderData.pRenderer;
		/*pRenderer->AcquireGeometryResources(pRenderer->GetDebugVertexBuffer(), pRenderer->GetDebugIndexBuffer(),
											4, sizeof(Vector), 0, ptVerts,
											6, indices, &newGeo.geometryInstance);*/

		// Create a transform matrix (this method assumes that this geometry will not be parented with another transform).
		Matrix4x4 matTransform{ 1.0f };
		matTransform = Matrix4x4::Translate(matTransform, ptOrigin);
		newGeo.geometryInstance.matLocalToWorldTransform = matTransform;

		return newGeo;
	}

	DebugGeometry_t	AddLine(Vector ptStartPos, Vector ptEndPos, Vector4D color, float flDuration)
	{
		// Initialize the local shader data if needed.
		if (!s_DebugShaderData.bDebugShaderDataInitialized)
		{
			InitializeDebugShaderData();
		}

		DebugGeometry_t newGeo{};
		newGeo.color = color;
		newGeo.flTimeToRemove = s_DebugShaderData.pRenderer->GetRendererTime() + flDuration;

		if (flDuration < 0.0f)
		{
			// A negative duration means 'never remove this geometry'.
			newGeo.flTimeToRemove = -1.0f;
		}

		// Acquire new shader instance resources.
		ShaderHandle_t hDebugShader = s_DebugShaderData.hDebugLineShader;
		s_DebugShaderData.pShaderSystem->AcquireShaderInstanceResources(hDebugShader, &newGeo.iInstanceID, "DebugLineInstance");

		Vector ptVerts[2] = { ptStartPos, ptEndPos };

		IRenderer* pRenderer = s_DebugShaderData.pRenderer;
		/*pRenderer->AcquireGeometryResources(pRenderer->GetDebugVertexBuffer(), pRenderer->GetDebugIndexBuffer(),
			2, sizeof(Vector), 0, ptVerts,
			0, nullptr, &newGeo.geometryInstance);*/

		// Just use an identity matrix for the transform (this shouldn't ever move around).
		newGeo.geometryInstance.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);

		return newGeo;
	}

	DebugGeometry_t AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration)
	{
		// Initialize the local shader data if needed.
		if (!s_DebugShaderData.bDebugShaderDataInitialized)
		{
			InitializeDebugShaderData();
		}

		DebugGeometry_t newGeo{};
		newGeo.color = color;
		newGeo.flTimeToRemove = s_DebugShaderData.pRenderer->GetRendererTime() + flDuration;

		if (flDuration < 0.0f)
		{
			// A negative duration means 'never remove this geometry'.
			newGeo.flTimeToRemove = -1.0f;
		}

		// Acquire new shader instance resources.
		ShaderHandle_t hDebugShader = s_DebugShaderData.hDebugLineShader;
		s_DebugShaderData.pShaderSystem->AcquireShaderInstanceResources(hDebugShader, &newGeo.iInstanceID, "DebugBBoxInstance");

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

		IRenderer* pRenderer = s_DebugShaderData.pRenderer;
		/*pRenderer->AcquireGeometryResources(pRenderer->GetDebugVertexBuffer(), pRenderer->GetDebugIndexBuffer(),
			iNumVertices, sizeof(Vector), 0, ptVertices,
			iNumIndices, indices, &newGeo.geometryInstance);*/

		// Store off the transform matrix.
		newGeo.geometryInstance.matLocalToWorldTransform = pGeometry.matLocalToWorldTransform;

		return newGeo;
	}

	static void InitializeDebugShaderData()
	{
		const EngineInterfaces_t* pSystems = g_pEngine->GetEngineInterfaces();
		s_DebugShaderData.pShaderSystem = pSystems->pShaderSystem;
		s_DebugShaderData.pRenderer = pSystems->pRenderer;

		ShaderHandle_t hDebugLineShader = s_DebugShaderData.pShaderSystem->AcquireShader("Debug.Line");
		s_DebugShaderData.hDebugLineShader = hDebugLineShader;
		s_DebugShaderData.iDebugLineIdx_Projection = s_DebugShaderData.pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "projection");
		s_DebugShaderData.iDebugLineIdx_View = s_DebugShaderData.pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "view");
		s_DebugShaderData.iDebugLineIdx_Color = s_DebugShaderData.pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "color");
		s_DebugShaderData.iDebugLineIdx_Model = s_DebugShaderData.pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "model");

		s_DebugShaderData.bDebugShaderDataInitialized = true;
	}

}; // namespace DebugRender
