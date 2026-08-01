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

struct MeshBase_t
{
	u32					iMeshIdx{ kInvalidID_U32 };
	char*				pszName{ nullptr };

	Vector				vecMinExtents{};
	Vector				vecMaxExtents{};

	Utl::CVector<MeshRenderData_t>	renderData{};

	Utl::CVector<Vector>			submeshMinExtents{};
	Utl::CVector<Vector>			submeshMaxExtents{};
	Utl::CVector<Vector>			submeshCenters{};

	// FIXME: Remove this.
	GeometryInstance_t	geometry{};
};

struct MeshInstance_t
{
	MeshHandle_t	hBaseMesh{ kInvalidID_U32 };
	u16				iInstanceIdx{ kInvalidID_U16 };

	Matrix4x4		matLocalToWorld{};
};

//-------------------------------------------------------------------------
// Purpose: Mesh System Configuration.
//-------------------------------------------------------------------------
struct MeshSystemConfig_t
{
	// The max number of meshes that can be registered with the mesh system.
	u64	MaxMeshes{ 0 };

	// A pointer to the render system.
	class IRenderer* pRenderer{ nullptr };
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
	
	virtual MeshHandle_t	AcquireMeshBase(const char* pszMeshName) = 0;
	virtual u32				AcquireMeshInstance(MeshHandle_t hBaseMesh, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) = 0;
	virtual MeshBase_t*		AcquireMeshBaseDataFromHandle(MeshHandle_t hMesh) const = 0;
	virtual MeshInstance_t* AcquireMeshInstanceDataFromHandle(u32 hMeshInstance) const = 0;

	virtual void			DrawGeometry(u32 hMeshInstance, class IShaderSystem* pShaderSystem) const = 0;
	virtual void			DrawGeometryBySubmesh(u32 hMeshInstance, u32 iSubmesh, class IShaderSystem* pShaderSystem) const = 0;

	// Mesh acquisition.
	// FIXME: Deprecate this.
	virtual MeshHandle_t	AcquireMeshInstance(const char* pszMeshName, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) = 0;
	virtual MeshHandle_t	AcquireMeshFromResource(MeshLoader::MeshResource_t& pResource, const char* pszMeshName, bool bAutoRelease) = 0;
	virtual Mesh*			AcquireMeshDataByHandle(MeshHandle_t hMesh) = 0;

	virtual MeshHandle_t	WrapGeometryInstanceInMesh(GeometryInstance_t* pGeometryInstance, const char* pszName, MaterialHandle_t hMaterial) = 0;

	// Mesh release.
	virtual void			ReleaseMeshInstance(MeshHandle_t hMesh) = 0;
};