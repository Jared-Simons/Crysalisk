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

class CMap
{
public:
	static bool InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource, CMap& pOutMap);

	void	UnloadMap();

	inline bool	IsMapLoaded() const { return m_bMapResourcesLoaded; }

	const char*									GetMapName() const { return m_pszMapName; }
	const Utl::CVector<Mesh>&					GetRenderablesFromScene() const;

private:
	char*								m_pszMapName{ nullptr };

	Utl::CVector<Mesh>					m_Meshes{};

	bool								m_bMapResourcesLoaded{ false };
};

bool LoadMap(const char* pszMapName, CMap& pOutMapResource);
