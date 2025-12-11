//=============================================================================
// Author: Jared Simons
// Date Created: 12/10/2025
// 
// Purpose: The mesh system.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Resources/Loaders/MeshLoader.h>
#include "Resources/ResourceTypes.h"

struct MeshSystemConfig_t
{
	size_t	MaxMeshes{ 0 };
};

class IMeshSystem
{
public:
	// Initialize, shutdown
	virtual bool			Initialize(const MeshSystemConfig_t& pConfig) = 0;
	virtual void			Shutdown() = 0;

	virtual MeshHandle_t	AcquireMesh(const char* pszMeshName, bool bAutoRelease) = 0;
};

extern IMeshSystem* g_pMeshSystem;
