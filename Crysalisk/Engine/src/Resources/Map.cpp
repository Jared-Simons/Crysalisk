//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Implementation of map/scene related functions.
//=============================================================================
#include "Map.h"

#include <Core/Logging.h>
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
		m_pMapName = Utl::StringDuplicate(pNewMapResource.pszMapName);
	}

	return true;
}

