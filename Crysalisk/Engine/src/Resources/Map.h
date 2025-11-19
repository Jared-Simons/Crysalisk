//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Interface for map/scene related functions.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include <Resources/Loaders/MapLoader.h>
#include "Resources/ResourceTypes.h"

// RAYTODO: Should this live somewhere else?
struct Ray_t
{
	Vector	vecStart{};
	Vector	vecDirection{};
	Vector	vecEnd{};
	bool	bDidHit{ false };
	float	flDistance{ 0.0f };
};

class CMap
{
public:
	static bool InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource, CMap& pOutMap);

	void	UnloadMap();

	inline bool	IsMapLoaded() const { return m_bMapResourcesLoaded; }

	const char*									GetMapName() const { return m_pszMapName; }
	void										GetRenderablesFromScene(Frustum& pCameraFrustum, Utl::CVector<Matrix4x4>& pTransformationMatrices, Utl::CVector<GeometryInstance_t*>& pVisibleOpaqueGeometries, Utl::CVector<GeometryInstance_t*>& pVisibleTranslucentGeometries, unsigned int* pCulledGeometries) const;
	void										CastRayIntoScene(Ray_t& pRay) const;

private:
	char*								m_pszMapName{ nullptr };

	Utl::CVector<Mesh>					m_Meshes{};

	bool								m_bMapResourcesLoaded{ false };
};

bool LoadMap(const char* pszMapName, CMap& pOutMapResource);
