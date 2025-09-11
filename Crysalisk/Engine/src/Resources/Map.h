//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Interface for map/scene related functions.
//=============================================================================
#pragma once

#include <Resources/Loaders/MapLoader.h>	// MAPTODO: We really should only need this Map.cpp

class CMap
{
public:
	bool	InitializeFromResource(struct MapLoader::MapResource_t& pNewMapResource);

private:
	const char* m_pMapName{ nullptr };
};

bool LoadMap(const char* pszMapName, CMap& pOutMapResource);
