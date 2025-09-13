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

	// Use the loaded resource to acquire assets.
	if (!pOutMapResource.InitializeFromResource(mapResource))
	{
		// TODO: Logging
		return false;
	}

	return true;
}

bool CMap::InitializeFromResource(MapLoader::MapResource_t& pNewMapResource)
{
	if (pNewMapResource.pszMapName)
	{
		m_pszMapName = Utl::StringDuplicate(pNewMapResource.pszMapName);
	}

	if (pNewMapResource.MeshResources.Count() > 0)
	{
		LOG_INFO("Acquiring map mesh resources.");

		size_t iMeshCount = pNewMapResource.MeshResources.Count();
		m_Meshes.Resize(iMeshCount);

		MeshLoader::MeshResource_t*						pCurObject = nullptr;
		MeshLoader::MeshResource_t::GeometryGroup_t*	pCurMesh = nullptr;
		MeshLoader::MeshResource_t::GeometryObject_t*	pCurSubMesh = nullptr;

		for (int objIdx = 0; objIdx < iMeshCount; objIdx++)
		{
			pCurObject = &pNewMapResource.MeshResources[objIdx];
			CAssert(pCurObject != nullptr);

			for (int meshIdx = 0; meshIdx < pCurObject->pObjects.Count(); meshIdx++)
			{
				pCurMesh = pCurObject->pObjects[meshIdx];
				CAssert(pCurMesh != nullptr);

				for (int subMeshIdx = 0; subMeshIdx < pCurMesh->pGroups.Count(); subMeshIdx++)
				{
					GeometryConfig_t newConfig{};
					newConfig.Vertices = pCurMesh->pGroups[subMeshIdx]->Vertices;
					newConfig.Indices = pCurMesh->pGroups[subMeshIdx]->Indices;
					if (pCurMesh->pGroups[subMeshIdx]->pszDebugName)
						newConfig.pszDebugName = Utl::StringDuplicate(pCurMesh->pGroups[subMeshIdx]->pszDebugName);

					if (newConfig.Vertices.Count() > 0)
					{
						LOG_VERBOSE("Acquiring geometry resources for sub-mesh '%s'.", pCurMesh->pGroups[subMeshIdx]->pszDebugName);

						m_Meshes.PushBack(GeometryInstance_t{});
						GeometryInstance_t& newGeometry = m_Meshes.Back();

						g_pRenderer->AcquireGeometryResources(newConfig, newGeometry);
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

	return true;
}

void CMap::Destroy()
{
	if (m_pszMapName)
	{
		Utl::StringFree(m_pszMapName);
		m_pszMapName = nullptr;
	}

	m_Meshes.Clear();
}

Utl::CVector<GeometryInstance_t>& CMap::GetRenderablesFromScene()
{
	return m_Meshes;
}

