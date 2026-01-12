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

//-------------------------------------------------------------------------
// Purpose: Mesh System Configuration.
//-------------------------------------------------------------------------
struct MeshSystemConfig_t
{
	// The max number of meshes that can be registered with the mesh system.
	size_t	MaxMeshes{ 0 };

	// A pointer to the render system.
	class Renderer* pRenderer{ nullptr };
	// A pointer to the job system.
	class IJobSystem* pJobSystem{ nullptr };
	// A pointer to the material system.
	class IMaterialSystem* pMaterialSystem{ nullptr };
};

//-------------------------------------------------------------------------
// Purpose: Mesh System Interface.
//-------------------------------------------------------------------------
class IMeshSystem
{
public:
	static void				AllocateMeshSystemState(IMeshSystem** ppSystemState);

	// Initialize, shutdown
	virtual bool			Initialize(const MeshSystemConfig_t& pAppConfig) = 0;
	virtual void			Shutdown() = 0;

	// Mesh acquisition.
	virtual MeshHandle_t	AcquireMeshInstance(const char* pszMeshName, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) = 0;
	virtual MeshHandle_t	AcquireMeshFromResource(MeshLoader::MeshResource_t& pResource, const char* pszMeshName, bool bAutoRelease) = 0;
	virtual Mesh*			AcquireMeshDataByHandle(MeshHandle_t hMesh) = 0;

	// Mesh release.
	virtual void			ReleaseMeshInstance(MeshHandle_t hMesh) = 0;
};
