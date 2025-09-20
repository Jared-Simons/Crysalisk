//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Interface for map/scene related functions.
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include "Resources/ResourceTypes.h"

class CMap
{
public:
	void	UnloadMap();

	inline bool	IsMapLoaded() const { return m_bMapResourcesLoaded; }

	Utl::CVector<GeometryInstance_t>&	GetRenderablesFromScene();

	// MAPTODO: Should these be private?
public:
	char* m_pszMapName{ nullptr };

	Utl::CVector<GeometryInstance_t>	m_Meshes{};

	bool	m_bMapResourcesLoaded{ false };
};

bool LoadMap(const char* pszMapName, CMap& pOutMapResource);
