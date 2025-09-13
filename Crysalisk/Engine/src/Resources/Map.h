//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Interface for map/scene related functions.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include <Resources/Loaders/MapLoader.h>	// MAPTODO: We really should only need this Map.cpp
#include "Resources/ResourceTypes.h"

class CMap
{
public:
	bool	InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource);

	void	Destroy();

	Utl::CVector<GeometryInstance_t>&	GetRenderablesFromScene();

private:
	char* m_pszMapName{ nullptr };

	Utl::CVector<GeometryInstance_t>	m_Meshes{};
};

bool LoadMap(const char* pszMapName, CMap& pOutMapResource);
