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
#include "Resources/DebugGeometry.h"
#include <Resources/Loaders/MapLoader.h>
#include "Resources/ResourceTypes.h"

//-------------------------------------------------------------------------
// Represents a ray which can collide with objects in a scene.
//-------------------------------------------------------------------------
struct Ray_t
{
	// The starting point of the ray.
	Vector	vecOrigin{};
	// The direction the ray travels.
	Vector	vecDirection{};
	// The endpoint of the ray.
	Vector	vecEnd{};
	// Indicates if the ray hit anything.
	bool	bDidHit{ false };
	// The distance the ray travelled from its origin along it's direction vector.
	float	flDistance{ 0.0f };
};

class CMap
{
public:
	static bool InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource, class IMeshSystem* pMeshSystem, class IMaterialSystem* pMaterialSystem, CMap& pOutMap);

	void	UnloadMap(class IMeshSystem* pMeshSystem);

	inline bool	IsMapLoaded() const { return m_bMapResourcesLoaded; }

	const char*									GetMapName() const { return m_pszMapName; }
	ENGINE_API void								GetRenderablesFromScene(class IMaterialSystem* pMaterialSystem, Frustum& pCameraFrustum, RenderPacket_t* pPacket, unsigned int* pCulledGeometries) const;
	void										CastRayIntoScene(Ray_t& pRay, class IDebugRenderer* pDebugRenderer) const;

	// FIXME: this should be private
	Utl::CVector<DebugRender::DebugGeometry_t>		m_hDebugMeshes{};
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

bool LoadMap(const char* pszMapName, IMeshSystem* pMeshSystem, IMaterialSystem* pMaterialSystem, CMap& pOutMapResource);
