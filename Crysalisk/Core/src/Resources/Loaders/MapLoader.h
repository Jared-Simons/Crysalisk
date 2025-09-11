//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Map resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Resources/Loaders/MeshLoader.h"
#include "Strings/String_Utils.h"

namespace MapLoader
{
	// Attempts to load an existing map asset from disk.
	CORE_API bool LoadMapResource(const char* pszMapFilePath, struct MapResource_t& pOutResource);

	struct MapResource_t
	{
		char* pszMapName{ nullptr };

		// Static meshes.
		Utl::CVector<MeshLoader::MeshResource_t>	MeshResources{};

		~MapResource_t()
		{
			if (pszMapName)
			{
				Utl::StringFree(pszMapName);
				pszMapName = nullptr;
			}
		}
	};
} // namespace MapLoader
