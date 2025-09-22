//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Implementation of map/scene related functions.
//=============================================================================
#include "Map.h"

#include <Core/Logging.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>
#include "Systems/MaterialSystem.h"

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
		// TODO: Logging
		return false;
	}

	LOG_DEBUG("Memory usage post-map resource load: %.5f", g_pMemoryManager->GetPercentAllocated());

	// Use the loaded resource to acquire assets.
	if (!CMap::InitializeFromResource(mapResource, pOutMapResource))
	{
		// TODO: Logging
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

		size_t iMeshCount = pNewMapResource.MeshResources.Capacity();
		pOutMap.m_Meshes.Resize(iMeshCount);

		MeshLoader::MeshResource_t*						pCurObject = nullptr;
		MeshLoader::MeshResource_t::GeometryGroup_t*	pCurMesh = nullptr;
		MeshLoader::MeshResource_t::GeometryObject_t*	pCurSubMesh = nullptr;

		for (int objIdx = 0; objIdx < iMeshCount; objIdx++)
		{
			pCurObject = &pNewMapResource.MeshResources[objIdx];
			CAssert(pCurObject != nullptr);

			for (int meshIdx = 0; meshIdx < pCurObject->pObjects.Capacity(); meshIdx++)
			{
				// Add a new mesh entry to the map.
				pOutMap.m_Meshes.PushBack(Mesh{});
				Mesh* pCurOutMesh = &pOutMap.m_Meshes.Back();

				// Set the transformation matrix.
				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);
				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4::Scale(pCurOutMesh->m_matLocalToWorldTransform, pCurObject->vecScale);

				Quaternion quatAngles = QuaternionFromEulerAngles(pCurObject->vecEulerAngles);
				Matrix4x4 matAngles = Mat4FromQuaternion(quatAngles);
				pCurOutMesh->m_matLocalToWorldTransform = pCurOutMesh->m_matLocalToWorldTransform * matAngles;

				pCurOutMesh->m_matLocalToWorldTransform = Matrix4x4::Translate(pCurOutMesh->m_matLocalToWorldTransform, pCurObject->vecPosition);

				pCurMesh = &pCurObject->pObjects[meshIdx];
				CAssert(pCurMesh != nullptr);

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

						g_pRenderer->AcquireGeometryResources(newConfig, newGeometry);

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

	m_Meshes.Clear();
}

const Utl::CVector<Mesh>& CMap::GetRenderablesFromScene() const
{
	return m_Meshes;
}

