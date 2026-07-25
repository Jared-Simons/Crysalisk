//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Interface for map/scene related functions.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include <Defines.h>
#include <Renderer/Camera.h>
#include <Resources/Loaders/MapLoader.h>
#include "Resources/ResourceTypes.h"

class CMap
{
public:
	static bool InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource, class IMeshSystem* pMeshSystem, class IMaterialSystem* pMaterialSystem, CMap& pOutMap);

	void	UnloadMap(class IMeshSystem* pMeshSystem);

	inline bool	IsMapLoaded() const { return m_bMapResourcesLoaded; }

	const char*									GetMapName() const { return m_pszMapName; }
	ENGINE_API void								GetRenderablesFromScene(class IMaterialSystem* pMaterialSystem, Frustum& pCameraFrustum, Utl::CVector<GeometryInstance_t*>& pOpaqueGeometries, Utl::CVector<GeometryInstance_t*>& pTransparentGeometries, u32* pCulledGeometries) const;
	void										CastRayIntoScene(struct Ray_t& pRay, class IDebugGeometrySystem* pDebugGeometry) const;

private:
	char*								m_pszMapName{ nullptr };

	struct MapMesh_t
	{
		MeshHandle_t	hMesh{ kInvalidID_U32 };
		const Mesh*		pMesh{ nullptr };
	};

	Utl::CVector<MapMesh_t>							m_hMeshes{};

	bool								m_bMapResourcesLoaded{ false };
};

ENGINE_API bool LoadMap(const char* pszMapName, class IMeshSystem* pMeshSystem, class IMaterialSystem* pMaterialSystem, CMap& pOutMapResource);
