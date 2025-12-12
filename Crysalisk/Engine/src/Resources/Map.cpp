//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Implementation of map/scene related functions.
//=============================================================================
#include "Map.h"

#include <Core/Logging.h>
#include <Math/MathLib.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>
#include "Systems/MaterialSystem.h"
#include "Systems/MeshSystem.h"
#include "Renderer/DebugRenderer.h"

bool LoadMap(const char* pszMapName, CMap& pOutMapResource)
{
	MapLoader::MapResource_t mapResource{};

	// Obviosuly we need something to load!
	if (!pszMapName)
	{
		return false;
	}

	LOG_DEBUG("Attempting to load map '%s'.", pszMapName);

	// Piece together the full path relative to our application's directory.
	char szFormattedMapPath[512]{};
	Utl::StringFormat_V("Assets/Maps/%s.cmap", szFormattedMapPath, pszMapName);

	// Attempt to load the resource from disk.
	if (!MapLoader::LoadMapResource(szFormattedMapPath, mapResource))
	{
		LOG_ERROR("Failed to load map '%s'.", pszMapName);
		return false;
	}

	LOG_DEBUG("Memory usage post-map resource load: %.5f", g_pMemoryManager->GetPercentAllocated());

	// Use the loaded resource to acquire assets.
	if (!CMap::InitializeFromResource(mapResource, pOutMapResource))
	{
		LOG_ERROR("Failed to initialize map '%s'.", pszMapName);
		return false;
	}

	LOG_DEBUG("Memory usage post-map load: %.5f", g_pMemoryManager->GetPercentAllocated());

	return true;
}

bool CMap::InitializeFromResource(MapLoader::MapResource_t& pNewMapResource, CMap& pOutMap)
{
	if (pNewMapResource.pszMapName)
	{
		pOutMap.m_pszMapName = Utl::StringDuplicate(pNewMapResource.pszMapName);
	}

	if (pNewMapResource.MeshResources_NEW.Count() > 0)
	{
		LOG_INFO("Acquiring map mesh resources.");

		pOutMap.m_hMeshes.Resize(pNewMapResource.MeshResources_NEW.Count());
		pOutMap.m_hMeshes.SetCount(pNewMapResource.MeshResources_NEW.Count());

		for (int iMesh = 0; iMesh < pNewMapResource.MeshResources_NEW.Count(); iMesh++)
		{
			MapLoader::MapResource_t::MeshDeclaration_t& pCurMeshDec = pNewMapResource.MeshResources_NEW[iMesh];
			LOG_INFO("%s", pCurMeshDec.pszModelName);

			// MESHSYSTEMTODO: Currently we need a pointer to the mesh so we can have DIRECT access to internal data (for frustum culling, which should be handled here,
			// not in the mesh system). We can't get the necessary data (extents) immediately because mesh loading is async now.
			// This feels very hacky, but maybe it will have more use later down the road?
			pOutMap.m_hMeshes[iMesh].hMesh = g_pMeshSystem->AcquireMeshInstance(pCurMeshDec.pszModelName, pCurMeshDec.vecPosition, pCurMeshDec.vecAngles, pCurMeshDec.vecScale, true);
			pOutMap.m_hMeshes[iMesh].pMesh = g_pMeshSystem->AcquireMeshDataByHandle(pOutMap.m_hMeshes[iMesh].hMesh);
		}
	}
	else
	{
		LOG_ERROR("Map '%s' contains no geometry to load!", pNewMapResource.pszMapName);
		return false;
	}

	// Setup light data.
	MapLoader::MapResource_t::LightResource_t& dirLight = pNewMapResource.DirectionalLight;
	MapLoader::MapResource_t::LightResource_t& pointLight1 = pNewMapResource.PointLights[0];
	MapLoader::MapResource_t::LightResource_t& pointLight2 = pNewMapResource.PointLights[1];
	MapLoader::MapResource_t::LightResource_t& pointLight3 = pNewMapResource.PointLights[2];
	MapLoader::MapResource_t::LightResource_t& pointLight4 = pNewMapResource.PointLights[3];
	MapLoader::MapResource_t::LightResource_t* spotLight = &pNewMapResource.SpotLight;

 	g_pMaterialSystemInternal->SetDirectionalLightData(dirLight.vecDirection, dirLight.lightColor, dirLight.flBrightness, dirLight.ambientLightColor, dirLight.flAmbientStrength);
	g_pMaterialSystemInternal->SetPointLightData(0, pointLight1.vecPosition, pointLight1.lightColor, pointLight1.flBrightness, pointLight1.flConstant, pointLight1.flLinear, pointLight1.flQuadratic);
	g_pMaterialSystemInternal->SetPointLightData(1, pointLight2.vecPosition, pointLight2.lightColor, pointLight2.flBrightness, pointLight2.flConstant, pointLight2.flLinear, pointLight2.flQuadratic);
	g_pMaterialSystemInternal->SetPointLightData(2, pointLight3.vecPosition, pointLight3.lightColor, pointLight3.flBrightness, pointLight3.flConstant, pointLight3.flLinear, pointLight3.flQuadratic);
	g_pMaterialSystemInternal->SetPointLightData(3, pointLight4.vecPosition, pointLight4.lightColor, pointLight4.flBrightness, pointLight4.flConstant, pointLight4.flLinear, pointLight4.flQuadratic);
	g_pMaterialSystemInternal->SetSpotLightData(spotLight->vecPosition, spotLight->vecDirection, spotLight->lightColor, spotLight->flBrightness, spotLight->flConstant, spotLight->flLinear, spotLight->flQuadratic, spotLight->flInnerCutoff, spotLight->flOuterCutoff);
	g_pMaterialSystemInternal->ApplyLightData();

	pOutMap.m_bMapResourcesLoaded = true;
	return true;
}

void CMap::UnloadMap()
{
	m_bMapResourcesLoaded = false;

	if (m_pszMapName)
	{
		Utl::StringFree(m_pszMapName);
		m_pszMapName = nullptr;
	}

	// Release all mesh/material references.
	for (int iMeshIdx = 0; iMeshIdx < m_hMeshes.Count(); iMeshIdx++)
	{
		g_pMeshSystem->ReleaseMesh(m_hMeshes[iMeshIdx].hMesh);
	}

	m_hMeshes.Clear();
}

void CMap::GetRenderablesFromScene(Frustum& pCameraFrustum, Utl::CVector<Matrix4x4>& pTransformationMatrices, Utl::CVector<GeometryInstance_t*>& pVisibleOpaqueGeometries, Utl::CVector<GeometryInstance_t*>& pVisibleTranslucentGeometries, unsigned int* pCulledGeometries) const
{
	const Mesh*			pCurrentMesh = nullptr;
	GeometryInstance_t* pCurrentSubmesh = nullptr;

	for (int iMesh = 0; iMesh < m_hMeshes.Count(); iMesh++)
	{
 		pCurrentMesh = m_hMeshes[iMesh].pMesh;
		CAssert(pCurrentMesh);

		// Split out transform matrices.
		pTransformationMatrices.PushBack(m_hMeshes[iMesh].pMesh->m_matLocalToWorldTransform);

		for (int iSubMesh = 0; iSubMesh < pCurrentMesh->m_Geometries.Count(); iSubMesh++)
		{
			pCurrentSubmesh = &pCurrentMesh->m_Geometries[iSubMesh];
			CAssert(pCurrentSubmesh);

			bool bIsInCameraFrustum = true;

			pCurrentSubmesh->matLocalToWorldTransform = pCurrentMesh->m_matLocalToWorldTransform;

			Vector3D vecCenter = pCurrentSubmesh->matLocalToWorldTransform * pCurrentSubmesh->vecCenter;
			Vector3D vecMaxExtents = pCurrentSubmesh->matLocalToWorldTransform * pCurrentSubmesh->vecMaxExtents;

			Vector3D vecHalfExtents = {
				(float)M_ABS(vecMaxExtents.x - vecCenter.x),
				(float)M_ABS(vecMaxExtents.y - vecCenter.y),
				(float)M_ABS(vecMaxExtents.z - vecCenter.z)
			};

			if (!pCameraFrustum.FrustumIntersectsAABB(vecCenter, vecHalfExtents))
			{
				bIsInCameraFrustum = false;
			}

			// If the geometry is within the camera's frustum, add it to the list to be rendered.
			if (bIsInCameraFrustum)
			{
				if (!pCurrentSubmesh->bIsTransparent)
				{
					pVisibleOpaqueGeometries.PushBack(pCurrentSubmesh);
					pVisibleOpaqueGeometries.Back()->m_MeshID = pCurrentMesh->m_MeshID;
				}
				else
				{
					pVisibleTranslucentGeometries.PushBack(pCurrentSubmesh);
					pVisibleOpaqueGeometries.Back()->m_MeshID = pCurrentMesh->m_MeshID;
				}
			}
			else
			{
				(*pCulledGeometries)++;
				continue;
			}
		}
	}
}

static unsigned int iCurrentRayDebugGeoID{ kInvalidID_U32 };

void CMap::CastRayIntoScene(Ray_t& pRay) const
{
	pRay.bDidHit = false;
	pRay.flDistance = kFLT_MAX;

	// Check every object and return the nearest intersection point of the first piece of geometry along the ray.
	// TODO: This would be GREATLY enhanced with a spatial-partitioning structure.
	const Mesh* pCurrentMesh = nullptr;
	GeometryInstance_t* pCurrentSubmesh = nullptr;
	unsigned int iHitMesh{ kInvalidID_U32 }, iHitSubmesh{ kInvalidID_U32 };

	for (int iMesh = 0; iMesh < m_hMeshes.Count(); iMesh++)
	{
		pCurrentMesh = m_hMeshes[iMesh].pMesh;
		CAssert(pCurrentMesh);

		for (int iSubMesh = 0; iSubMesh < pCurrentMesh->m_Geometries.Count(); iSubMesh++)
		{
			pCurrentSubmesh = &pCurrentMesh->m_Geometries[iSubMesh];
			CAssert(pCurrentSubmesh);

			// We will perform the intersection test in the geometry's local space so that we can do this with an AABB test.
			// Start by transforming the given ray into local space.

			// NOTE: Explicitly zero-ing out the w in the direction so it transforms properly.
			Vector4D vecLocalDirection = pRay.vecDirection.ToVector4D();
			vecLocalDirection.w = 0.0f;

			Ray_t localRay{};
			localRay.vecOrigin = pCurrentSubmesh->matLocalToWorldTransform.Inverse() * pRay.vecOrigin;
			localRay.vecDirection = (pCurrentSubmesh->matLocalToWorldTransform.Inverse() * vecLocalDirection).AsVector3D();

			// These properties are already in local space.
			Vector vecCenter = pCurrentSubmesh->vecCenter;
			Vector vecMins = pCurrentSubmesh->vecMinExtents;
			Vector vecMaxs = pCurrentSubmesh->vecMaxExtents;

			float flTMin = kFLT_MAX;
			float flTMax = kFLT_MIN;

			Vector tMin = (vecMins - localRay.vecOrigin) / localRay.vecDirection;
			Vector tMax = (vecMaxs - localRay.vecOrigin) / localRay.vecDirection;

			Vector tNear = { FloatMin(tMin.x, tMax.x), FloatMin(tMin.y, tMax.y), FloatMin(tMin.z, tMax.z) };
			Vector tFar = { FloatMax(tMin.x, tMax.x), FloatMax(tMin.y, tMax.y), FloatMax(tMin.z, tMax.z) };

			float flTNearMax = FloatMax(FloatMax(tNear.x, tNear.y), tNear.z);
			float flTFarMin = FloatMin(FloatMin(tFar.x, tFar.y), tFar.z);

			if ((flTNearMax <= flTFarMin) && (flTNearMax < pRay.flDistance) && (flTNearMax > 0.0f))	// NOTE: In this particular test, the distance should always be > 0, as the objects in the view frustum should be along the front of the ray
			{																						// (the transforms involved in calculating the ray direction already accounts for Camera Forward = -Z axis).
 				pRay.bDidHit = true;
				pRay.flDistance = flTNearMax;
				pRay.vecEnd = pRay.vecOrigin + pRay.vecDirection * pRay.flDistance;
				iHitMesh = iMesh;
				iHitSubmesh = iSubMesh;
			}
		}
	}

	// Outline the geometry that we hit.
	if (iHitMesh != kInvalidID_U32 && iHitSubmesh != kInvalidID_U32)
	{
		if (iCurrentRayDebugGeoID != kInvalidID_U32)
			g_pDebugRenderer->RemoveDebugGeometry(iCurrentRayDebugGeoID);

		GeometryInstance_t& pGeometry = m_hMeshes[iHitMesh].pMesh->m_Geometries[iHitSubmesh];
		iCurrentRayDebugGeoID = g_pDebugRenderer->AddBBox(pGeometry, Vector4D{ 0.0f, 0.0f, 1.0f, 1.0f }, -1.0f);
	}
	if (!pRay.bDidHit)
	{
		// Just cast the ray off to the limits of the space.
		pRay.vecEnd = pRay.vecOrigin + pRay.vecDirection * pRay.flDistance;
	}
}

