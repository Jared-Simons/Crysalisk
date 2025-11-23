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

	if (pNewMapResource.MeshResources.Count() > 0)
	{
		LOG_INFO("Acquiring map mesh resources.");

		size_t iMeshCount = pNewMapResource.MeshResources.Count();
		pOutMap.m_Meshes.Resize(iMeshCount);

		MeshLoader::MeshResource_t*						pCurObject = nullptr;
		MeshLoader::MeshResource_t::GeometryGroup_t*	pCurMesh = nullptr;
		MeshLoader::MeshResource_t::GeometryObject_t*	pCurSubMesh = nullptr;

		for (int objIdx = 0; objIdx < iMeshCount; objIdx++)
		{
			pCurObject = &pNewMapResource.MeshResources[objIdx];
			CAssert(pCurObject != nullptr);

			for (int meshIdx = 0; meshIdx < pCurObject->pObjects.Count(); meshIdx++)
			{
				// Add a new mesh entry to the map.
				pOutMap.m_Meshes.PushBack(Mesh{});
				Mesh* pCurOutMesh = &pOutMap.m_Meshes.Back();
				pCurOutMesh->m_MeshID = (uint32_t)pOutMap.m_Meshes.Count();

				// Set the transformation matrix.
				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4{ 1.0f };
				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4::Scale(pCurOutMesh->m_matLocalToWorldTransform, pCurObject->vecScale);

				Quaternion quatAngles = QuaternionFromEulerAngles(pCurObject->vecEulerAngles);
				Matrix4x4 matAngles = Matrix4x4::Mat4FromQuaternion(quatAngles);
				pCurOutMesh->m_matLocalToWorldTransform = pCurOutMesh->m_matLocalToWorldTransform * matAngles;

				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4::Translate(pCurOutMesh->m_matLocalToWorldTransform, pCurObject->vecPosition);

				pCurMesh = &pCurObject->pObjects[meshIdx];
				CAssert(pCurMesh != nullptr);

				// TODO: Make this count, not capacity.
				for (int subMeshIdx = 0; subMeshIdx < pCurMesh->pGroups.Capacity(); subMeshIdx++)
				{
					GeometryConfig_t newConfig{};
					newConfig.Vertices = pCurMesh->pGroups[subMeshIdx].Vertices;
					newConfig.Indices = pCurMesh->pGroups[subMeshIdx].Indices;
					if (pCurMesh->pGroups[subMeshIdx].pszDebugName)
						newConfig.pszDebugName = Utl::StringDuplicate(pCurMesh->pGroups[subMeshIdx].pszDebugName);

					if (newConfig.Vertices.Count() > 0)
					{
						LOG_VERBOSE("Acquiring geometry resources for sub-mesh '%s'.", pCurMesh->pGroups[subMeshIdx].pszDebugName);

						pOutMap.m_Meshes.Back().m_Geometries.PushBack(GeometryInstance_t{});
						GeometryInstance_t& newGeometry = pOutMap.m_Meshes.Back().m_Geometries.Back();

						// Set up extent data.
						{
							// TODO: Use some actual limits for these values.
							Vector3D vecMinAABB{ 99999999.0f, 99999999.0f, 99999999.0f };
							Vector3D vecMaxAABB{ -99999999.0f, -99999999.0f, -99999999.0f };
							
							for (int vertIdx = 0; vertIdx < newConfig.Vertices.Count(); vertIdx++)
							{
								vecMinAABB.x = FloatMin(vecMinAABB.x, newConfig.Vertices[vertIdx].Pos.x);
								vecMinAABB.y = FloatMin(vecMinAABB.y, newConfig.Vertices[vertIdx].Pos.y);
								vecMinAABB.z = FloatMin(vecMinAABB.z, newConfig.Vertices[vertIdx].Pos.z);

								vecMaxAABB.x = FloatMax(vecMaxAABB.x, newConfig.Vertices[vertIdx].Pos.x);
								vecMaxAABB.y = FloatMax(vecMaxAABB.y, newConfig.Vertices[vertIdx].Pos.y);
								vecMaxAABB.z = FloatMax(vecMaxAABB.z, newConfig.Vertices[vertIdx].Pos.z);
							}

							newGeometry.vecMinExtents = vecMinAABB;
							newGeometry.vecMaxExtents = vecMaxAABB;
							newGeometry.vecCenter = (newGeometry.vecMinExtents + newGeometry.vecMaxExtents) * 0.5f;
						}

						g_pRenderer->AcquireGeometryResources(&g_pRenderer->m_WorldVertexBuffer, &g_pRenderer->m_WorldIndexBuffer, newConfig, newGeometry);

						// Acquire material resources.
						if (pCurMesh->pGroups[subMeshIdx].pszMaterialName != nullptr)
						{
							newGeometry.hMaterial = g_pMaterialSystemInternal->AcquireMaterial(pCurMesh->pGroups[subMeshIdx].pszMaterialName);

							// Set transparency state.
							newGeometry.bIsTransparent = g_pMaterialSystemInternal->IsMaterialTransparent(newGeometry.hMaterial);
						}
					}
				}
			}
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

	// Release all material references.
	for (int iMeshIdx = 0; iMeshIdx < m_Meshes.Count(); iMeshIdx++)
	{
		for (int iSubmeshIdx = 0; iSubmeshIdx < m_Meshes[iMeshIdx].m_Geometries.Count(); iSubmeshIdx++)
		{
			g_pMaterialSystemInternal->ReleaseMaterial(m_Meshes[iMeshIdx].m_Geometries[iSubmeshIdx].hMaterial, false);
		}
	}

	m_Meshes.Clear();
}

void CMap::GetRenderablesFromScene(Frustum& pCameraFrustum, Utl::CVector<Matrix4x4>& pTransformationMatrices, Utl::CVector<GeometryInstance_t*>& pVisibleOpaqueGeometries, Utl::CVector<GeometryInstance_t*>& pVisibleTranslucentGeometries, unsigned int* pCulledGeometries) const
{
	Mesh*				pCurrentMesh = nullptr;
	GeometryInstance_t* pCurrentSubmesh = nullptr;

	for (int iMesh = 0; iMesh < m_Meshes.Count(); iMesh++)
	{
 		pCurrentMesh = &m_Meshes[iMesh];
		CAssert(pCurrentMesh);

		// Split out transform matrices.
		pTransformationMatrices.PushBack(m_Meshes[iMesh].m_matLocalToWorldTransform);

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
	pRay.flDistance = 100000000000.0f; // RAYTODO: Some real limit here.

	// Check every object and return the nearest intersection point of the first piece of geometry along the ray.
	// TODO: This would be GREATLY enhanced with a spatial-partitioning structure.
	Mesh* pCurrentMesh = nullptr;
	GeometryInstance_t* pCurrentSubmesh = nullptr;
	unsigned int iHitMesh{ kInvalidID_U32 }, iHitSubmesh{ kInvalidID_U32 };

	for (int iMesh = 0; iMesh < m_Meshes.Count(); iMesh++)
	{
		pCurrentMesh = &m_Meshes[iMesh];
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
			localRay.vecStart = pCurrentSubmesh->matLocalToWorldTransform.Inverse() * pRay.vecStart;
			localRay.vecDirection = (pCurrentSubmesh->matLocalToWorldTransform.Inverse() * vecLocalDirection).AsVector3D();

			// These properties are already in local space.
			Vector vecCenter = pCurrentSubmesh->vecCenter;
			Vector vecMins = pCurrentSubmesh->vecMinExtents;
			Vector vecMaxs = pCurrentSubmesh->vecMaxExtents;

			// RAYTODO: Some actual limits here pls.
			float flTMin = 100000000000.0f;
			float flTMax = -100000000000.0f;

			Vector tMin = (vecMins - localRay.vecStart) / localRay.vecDirection;
			Vector tMax = (vecMaxs - localRay.vecStart) / localRay.vecDirection;

			Vector tNear = { FloatMin(tMin.x, tMax.x), FloatMin(tMin.y, tMax.y), FloatMin(tMin.z, tMax.z) };
			Vector tFar = { FloatMax(tMin.x, tMax.x), FloatMax(tMin.y, tMax.y), FloatMax(tMin.z, tMax.z) };

			float flTNearMax = FloatMax(FloatMax(tNear.x, tNear.y), tNear.z);
			float flTFarMin = FloatMin(FloatMin(tFar.x, tFar.y), tFar.z);

			if ((flTNearMax <= flTFarMin) && (flTNearMax < pRay.flDistance) && (flTNearMax > 0.0f))	// NOTE: In this particular test, the distance should always be > 0, as the objects in the view frustum should be along the front of the ray
			{																						// (the transforms involved in calculating the ray direction already accounts for Camera Forward = -Z axis).
 				pRay.bDidHit = true;
				pRay.flDistance = flTNearMax;
				pRay.vecEnd = pRay.vecStart + pRay.vecDirection * pRay.flDistance;
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

		GeometryInstance_t& pGeometry = m_Meshes[iHitMesh].m_Geometries[iHitSubmesh];
		iCurrentRayDebugGeoID = g_pDebugRenderer->AddBBox(pGeometry, Vector4D{ 0.0f, 0.0f, 1.0f, 1.0f }, -1.0f);
	}
	if (!pRay.bDidHit)
	{
		// Just cast the ray off to the limits of the space.
		pRay.vecEnd = pRay.vecStart + pRay.vecDirection * pRay.flDistance;
	}
}

