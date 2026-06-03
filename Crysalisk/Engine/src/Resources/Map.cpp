//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Implementation of map/scene related functions.
//=============================================================================
#include "Map.h"

#include <Containers/CVector.h>
#include <Core/Logging.h>
#include "Core/Engine.h"
#include <Math/MathLib.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>
#include "Systems/DebugGeometrySystem.h"
#include "Systems/MaterialSystem.h"
#include "Systems/MeshSystem.h"

bool LoadMap(const char* pszMapName, IMeshSystem* pMeshSystem, IMaterialSystem* pMaterialSystem, CMap& pOutMapResource)
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
	if (!CMap::InitializeFromResource(mapResource, pMeshSystem, pMaterialSystem, pOutMapResource))
	{
		LOG_ERROR("Failed to initialize map '%s'.", pszMapName);
		return false;
	}

	LOG_DEBUG("Memory usage post-map load: %.5f", g_pMemoryManager->GetPercentAllocated());

	return true;
}

bool CMap::InitializeFromResource(MapLoader::MapResource_t& pNewMapResource, IMeshSystem* pMeshSystem, IMaterialSystem* pMaterialSystem, CMap& pOutMap)
{
	if (pNewMapResource.pszMapName)
	{
		pOutMap.m_pszMapName = Utl::StringDuplicate(pNewMapResource.pszMapName);
	}

	if (pNewMapResource.MeshResources.Count() > 0)
	{
		LOG_INFO("Acquiring map mesh resources.");

		pOutMap.m_hMeshes.Resize(pNewMapResource.MeshResources.Count());

		for (int iMesh = 0; iMesh < pNewMapResource.MeshResources.Count(); iMesh++)
		{
			MapLoader::MapResource_t::MeshDeclaration_t& pCurMeshDec = pNewMapResource.MeshResources[iMesh];

			// NOTE: Currently we need a pointer to the mesh so we can have DIRECT access to internal data (for frustum culling, which should be handled here,
			// not in the mesh system). We can't get the necessary data (extents) immediately because mesh loading is async now.
			pOutMap.m_hMeshes[iMesh].hMesh = pMeshSystem->AcquireMeshInstance(pCurMeshDec.pszModelName, pCurMeshDec.vecPosition, pCurMeshDec.vecAngles, pCurMeshDec.vecScale, true);
			pOutMap.m_hMeshes[iMesh].pMesh = pMeshSystem->AcquireMeshDataByHandle(pOutMap.m_hMeshes[iMesh].hMesh);
		}
		pOutMap.m_hMeshes.SetCount(pNewMapResource.MeshResources.Count());
	}
	else
	{
		LOG_ERROR("Map '%s' contains no geometry to load!", pNewMapResource.pszMapName);
		return false;
	}

	// Setup light data.
	MapLoader::MapResource_t::LightResource_t& dirLight = pNewMapResource.DirectionalLight;
	pMaterialSystem->SetDirectionalLightData(dirLight.vecDirection, dirLight.lightColor, dirLight.flBrightness, dirLight.ambientLightColor, dirLight.flAmbientStrength);

	if (pNewMapResource.PointLights.Count() > 0)
	{
		MapLoader::MapResource_t::LightResource_t& pointLight1 = pNewMapResource.PointLights[0];
		MapLoader::MapResource_t::LightResource_t& pointLight2 = pNewMapResource.PointLights[1];
		MapLoader::MapResource_t::LightResource_t& pointLight3 = pNewMapResource.PointLights[2];
		MapLoader::MapResource_t::LightResource_t& pointLight4 = pNewMapResource.PointLights[3];

		pMaterialSystem->SetPointLightData(0, pointLight1.vecPosition, pointLight1.lightColor, pointLight1.flBrightness, pointLight1.flConstant, pointLight1.flLinear, pointLight1.flQuadratic);
		pMaterialSystem->SetPointLightData(1, pointLight2.vecPosition, pointLight2.lightColor, pointLight2.flBrightness, pointLight2.flConstant, pointLight2.flLinear, pointLight2.flQuadratic);
		pMaterialSystem->SetPointLightData(2, pointLight3.vecPosition, pointLight3.lightColor, pointLight3.flBrightness, pointLight3.flConstant, pointLight3.flLinear, pointLight3.flQuadratic);
		pMaterialSystem->SetPointLightData(3, pointLight4.vecPosition, pointLight4.lightColor, pointLight4.flBrightness, pointLight4.flConstant, pointLight4.flLinear, pointLight4.flQuadratic);
	}
	else
	{
		// TODO: If no lights are present in the map file, default to fullbright?

		// If there is no lighting data present, we need to make sure to default everything to 0 EXCEPT for the quadratic attenuation term!
		// If all 3 attenuation terms are 0, then there'll be NaNs in the shader. We'll use the standard 1-quadratic as a default.
		Vector vecZero(0.0f, 0.0f, 0.0f);
		pMaterialSystem->SetPointLightData(0, vecZero, vecZero, 0.0f, 0.0f, 0.0f, 1.0f);
		pMaterialSystem->SetPointLightData(1, vecZero, vecZero, 0.0f, 0.0f, 0.0f, 1.0f);
		pMaterialSystem->SetPointLightData(2, vecZero, vecZero, 0.0f, 0.0f, 0.0f, 1.0f);
		pMaterialSystem->SetPointLightData(3, vecZero, vecZero, 0.0f, 0.0f, 0.0f, 1.0f);
	}
	
	MapLoader::MapResource_t::LightResource_t* spotLight = &pNewMapResource.SpotLight;
	if ((spotLight->flLinear < kVectorElementEpsilon) && (spotLight->flConstant < kVectorElementEpsilon) && (spotLight->flQuadratic < kVectorElementEpsilon))	// No NaNs!
		spotLight->flQuadratic = 1.0f;

	pMaterialSystem->SetSpotLightData(spotLight->vecPosition, spotLight->vecDirection, spotLight->lightColor, spotLight->flBrightness, spotLight->flConstant, spotLight->flLinear, spotLight->flQuadratic, spotLight->flInnerCutoff, spotLight->flOuterCutoff);
	pMaterialSystem->ApplyLightData();

	pOutMap.m_bMapResourcesLoaded = true;
	return true;
}

void CMap::UnloadMap(IMeshSystem* pMeshSystem)
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
		pMeshSystem->ReleaseMeshInstance(m_hMeshes[iMeshIdx].hMesh);
	}

	m_hMeshes.Clear();
}

void CMap::GetRenderablesFromScene(class IMaterialSystem* pMaterialSystem, Frustum& pCameraFrustum, Utl::CVector<GeometryInstance_t*>& pOpaqueGeometries, Utl::CVector<GeometryInstance_t*>& pTransparentGeometries, unsigned int* pCulledGeometries) const
{
	const Mesh*			pCurrentMesh = nullptr;
	GeometryInstance_t* pCurrentSubmesh = nullptr;

	for (int iMesh = 0; iMesh < m_hMeshes.Count(); iMesh++)
	{
 		pCurrentMesh = m_hMeshes[iMesh].pMesh;
		CAssert(pCurrentMesh);

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
				// TODO: is there a good way to cache this state off instead of querying it everytime?
				if (!pMaterialSystem->IsMaterialTransparent(pCurrentSubmesh->hMaterial))
				{
					pOpaqueGeometries.PushBack(pCurrentSubmesh);
					pOpaqueGeometries.Back()->m_MeshID = pCurrentMesh->m_MeshID;
				}
				else
				{
					pTransparentGeometries.PushBack(pCurrentSubmesh);
					pTransparentGeometries.Back()->m_MeshID = pCurrentMesh->m_MeshID;
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

void CMap::CastRayIntoScene(Ray_t& pRay, IDebugGeometrySystem* pDebugGeometry) const
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
				pRay.pHitGeometry = pCurrentSubmesh;
				iHitMesh = iMesh;
				iHitSubmesh = iSubMesh;
			}
		}
	}
}

