//=============================================================================
// Author: Jared Simons
// Date Created: 12/10/2025
// 
// Purpose: The mesh system.
//=============================================================================
#include "MeshSystem.h"

#include <Containers/CHashMap.h>
#include "Renderer/Renderer.h"
#include "Systems/JobSystem.h"
#include "Systems/MaterialSystem.h"
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Purpose: Mesh System Implementation.
//-------------------------------------------------------------------------
class CMeshSystem : public IMeshSystem
{
public:
	virtual bool			Initialize(const MeshSystemConfig_t& pConfig) override;
	virtual void			Shutdown() override;

	virtual MeshHandle_t	AcquireMeshInstance(const char* pszMeshName, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) override;
	virtual MeshHandle_t	AcquireMeshFromResource(MeshLoader::MeshResource_t& pResource, const char* pszMeshName, bool bAutoRelease) override;
	virtual Mesh*			AcquireMeshDataByHandle(MeshHandle_t hMesh) override;

	virtual void			ReleaseMeshInstance(MeshHandle_t hMesh) override;

private:
	MeshHandle_t			GetFirstFreeMeshHandle() const;

	void					ClearMeshDictionaryEntry(const char* pszMeshPath);

private:
	MeshSystemConfig_t	m_Config{};

	struct MeshLookup
	{
		MeshHandle_t	MeshHandle{ kInvalidID_U32 };
		int				iReferenceCount{ 0 };
		bool			bAutoRelease{ false };
	};

	struct MeshDictionary_t
	{
		Utl::CStringHashMap<MeshLookup>	m_MeshHashmap{};
		Mesh*							m_MeshArray{ nullptr };
	};

	// The mesh dictionary.
	MeshDictionary_t	m_MeshDictionary{};
};

// Mesh System Context
static CMeshSystem s_MeshSystem;
IMeshSystem* g_pMeshSystem = &s_MeshSystem;

//-------------------------------------------------------------------------
// Purpose: Invoked upon successful completion of an async mesh-loading job.
//-------------------------------------------------------------------------
static int OnMeshLoadJob_Success(JobResults_t& pResults)
{
	MeshLoader::MeshLoaderParams_t* pTypedResults = reinterpret_cast<MeshLoader::MeshLoaderParams_t*>(pResults.pUserData);

	MeshLoader::MeshResource_t::GeometryGroup_t*	pMesh = &pTypedResults->pOutResource->pObjects[0];
	MeshLoader::MeshResource_t::GeometryObject_t*	pSubmesh = nullptr;

	// This will handle mesh registration and GPU upload.
	s_MeshSystem.AcquireMeshFromResource(*pTypedResults->pOutResource, pTypedResults->pszMeshPath, true);

	// Release all the resources allocated for this job.
	delete pTypedResults->pOutResource;
	pTypedResults->pOutResource = nullptr;
	CAssert(pTypedResults->pszMeshPath != nullptr);
	Utl::StringFree(pTypedResults->pszMeshPath);
	pTypedResults->pszMeshPath = nullptr;
	delete pTypedResults;
	pTypedResults = nullptr;

	return 1;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMeshSystem::Initialize(const MeshSystemConfig_t& pConfig)
{
	if (pConfig.MaxMeshes < 1)
	{
		LOG_ERROR("Mesh System REQUIRES Config.MeshMeshes > 0.");
		return false;
	}

	m_Config = pConfig;

	m_MeshDictionary.m_MeshArray = new Mesh[m_Config.MaxMeshes];
	m_MeshDictionary.m_MeshHashmap.Resize(m_Config.MaxMeshes);

	LOG_INFO("Mesh system initialized! (Max meshes: %d)", m_Config.MaxMeshes);
	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMeshSystem::Shutdown()
{
	if (m_MeshDictionary.m_MeshArray)
	{
		delete[] m_MeshDictionary.m_MeshArray;
		m_MeshDictionary.m_MeshArray = nullptr;
	}
}

//-------------------------------------------------------------------------
// Purpose: Attempts to acquire the resources for a mesh.
// Input:	pszMeshName - The name of the mesh to load.
//			vecPos - The position in worldspace to place this mesh.
//			vecEulerAngles - The angles to rotate this mesh.
//			vecScale - The scale factor for this mesh.
//			bAutoRelease - If true, the mesh will release when it's reference count reaches 0.
// Output:	Returns a handle to the mesh.
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::AcquireMeshInstance(const char* pszMeshName, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease)
{
	if (!pszMeshName)
	{
		LOG_ERROR("CMeshSystem::AcquireMeshInstance requires the name of a mesh to load. Nothing was done.");
		return kInvalidID_U32;
	}

	MeshHandle_t	newMeshHandle = kInvalidID_U32;
	MeshLookup*		pMeshLookup = nullptr;
	Mesh*			pMesh = nullptr;

	MeshLoader::MeshResource_t* pMeshResource = new MeshLoader::MeshResource_t;
	MeshLoader::MeshLoaderParams_t* pMeshParams = new MeshLoader::MeshLoaderParams_t;

	// Format the string with the path to the model directory.
	char szFormattedPath[512]{};
	Utl::StringFormat_V("Assets/Models/%s\0", szFormattedPath, pszMeshName);

	pMeshLookup = m_MeshDictionary.m_MeshHashmap.Insert(szFormattedPath);

	if (pMeshLookup->MeshHandle == kInvalidID_U32)
	{
		// The mesh hasn't been loaded yet. Load it up!
		newMeshHandle = GetFirstFreeMeshHandle();
		if (newMeshHandle == kInvalidID_U32)
		{
			LOG_ERROR("AcquireMeshInstance() could not acquire a new mesh slot. Increase mesh system config to allow for more. Returning null handle.");
			return kInvalidID_U32;
		}

		pMesh = &m_MeshDictionary.m_MeshArray[newMeshHandle];
		pMesh->m_MeshID = newMeshHandle;

		pMesh->m_pszMeshPath = Utl::StringDuplicate(szFormattedPath);

		pMesh->m_vecPosition = vecPos;
		pMesh->m_vecAngles = vecEulerAngles;
		pMesh->m_vecScale = vecScale;

		pMeshLookup->MeshHandle = newMeshHandle;

		pMeshParams->pszMeshPath = Utl::StringDuplicate(szFormattedPath);
		pMeshParams->pOutResource = pMeshResource;

		JobDeclaration_t meshLoadJob{};
		meshLoadJob.pfnEntryPoint = MeshLoader::LoadMesh_Threaded;
		meshLoadJob.pParams = pMeshParams;
		meshLoadJob.iParamSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.pJobResults.pfnOnJobCompleted_Success_Callback = OnMeshLoadJob_Success;
		meshLoadJob.pJobResults.iUserDataSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.iJobType = WORKER_THREAD_TYPE_DISK_LOADING;
		meshLoadJob.ePriority = eJobPriority::NORMAL;

		g_pJobSystem->SubmitJob(meshLoadJob);
	}
	else
	{
		// The mesh is already loaded. Increment the reference count and return the handle.
		pMeshLookup->iReferenceCount++;
		newMeshHandle = pMeshLookup->MeshHandle;
	}

	return newMeshHandle;
}

//-------------------------------------------------------------------------
// Purpose: Creates a usable mesh given a loaded resource.
// Input:	pResource - The mesh's resource data loaded from disk.
//			pszMeshName - The name of the mesh being loaded.
//			bAutoRelease - If true, the mesh will release when it's reference count reaches 0.
// Output:	Returns a handle to the mesh.
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::AcquireMeshFromResource(MeshLoader::MeshResource_t& pResource, const char* pszMeshName, bool bAutoRelease)
{
	MeshLookup*		pMeshLookup = &m_MeshDictionary.m_MeshHashmap[pszMeshName];
	Mesh*			pMesh = nullptr;
	
	CAssert(pMeshLookup->MeshHandle != kInvalidID_U32);

	MeshHandle_t	meshHandle = pMeshLookup->MeshHandle;
	pMesh = &m_MeshDictionary.m_MeshArray[meshHandle];
	CAssert(pMesh);

	// Set the transformation matrix.
	pMesh->m_matLocalToWorldTransform = Matrix4x4{ 1.0f };
	pMesh->m_matLocalToWorldTransform = Matrix4x4::Scale(pMesh->m_matLocalToWorldTransform, pMesh->m_vecScale);

	Quaternion quatAngles = QuaternionFromEulerAngles(pMesh->m_vecAngles);
	Matrix4x4 matAngles = Matrix4x4::Mat4FromQuaternion(quatAngles);
	pMesh->m_matLocalToWorldTransform = pMesh->m_matLocalToWorldTransform * matAngles;

	pMesh->m_matLocalToWorldTransform = Matrix4x4::Translate(pMesh->m_matLocalToWorldTransform, pMesh->m_vecPosition);

	MeshLoader::MeshResource_t::GeometryObject_t* pCurSubmesh = nullptr;
	// Acquire renderer resources and upload to the GPU.
	// 
	// TODO: Only accounts for one mesh.
	// Reformat the mesh file eventally to make this cleaner.
	for (int iSubmesh = 0; iSubmesh < pResource.pObjects[0].pGroups.Capacity(); iSubmesh++)
	{
		pCurSubmesh = &pResource.pObjects[0].pGroups[iSubmesh];
		CAssert(pCurSubmesh);

		if (pCurSubmesh->Vertices.Count() < 1)
			continue;	// Vertex data is required!

		LOG_DEBUG("Acquiring mesh resources for: %s", pCurSubmesh->pszDebugName);

		GeometryConfig_t newConfig{};
		newConfig.Vertices = pCurSubmesh->Vertices;
		newConfig.Indices = pCurSubmesh->Indices;
		pMesh->m_Geometries.PushBack(GeometryInstance_t{});
		g_pRenderer->AcquireGeometryResources(&g_pRenderer->m_WorldVertexBuffer, &g_pRenderer->m_WorldIndexBuffer, newConfig, pMesh->m_Geometries.Back());

		GeometryInstance_t& pNewSubmesh = pMesh->m_Geometries.Back();

		pNewSubmesh.vecMinExtents = pCurSubmesh->vecMinExtents;
		pNewSubmesh.vecMaxExtents = pCurSubmesh->vecMaxExtents;
		pNewSubmesh.vecCenter = pCurSubmesh->vecCenter;

		// Acquire material resources.
		if (pCurSubmesh->pszMaterialName != nullptr)
		{
			pNewSubmesh.hMaterial = g_pMaterialSystemInternal->AcquireMaterial(pCurSubmesh->pszMaterialName);

			// Set transparency state.
			pNewSubmesh.bIsTransparent = g_pMaterialSystemInternal->IsMaterialTransparent(pNewSubmesh.hMaterial);
		}
	}

	// Increment the reference count.
	pMeshLookup->iReferenceCount++;
	pMeshLookup->bAutoRelease = bAutoRelease;

	return meshHandle;
}

//-------------------------------------------------------------------------
// Purpose: Returns usable internal data for a mesh, given it's handle.
// 
// NOTE:	This is needed because some systems need read-access to mesh internals
//			and it is more convenient to give them that access instead of making them go through here.
//			(Think of the map needing mesh extents for frustum-culling, for example).
// NOTE 2:	This DOES NOT increment the mesh's reference count.
// 
// Input:	hMesh - The mesh to return the internal data for.
// Output:	Returns a pointer to the mesh's internal data.
//-------------------------------------------------------------------------
Mesh* CMeshSystem::AcquireMeshDataByHandle(MeshHandle_t hMesh)
{
	CAssert(hMesh != kInvalidID_U32);

	if (m_MeshDictionary.m_MeshArray[hMesh].m_MeshID == kInvalidID_U32)
	{
		LOG_ERROR("Mesh with handle '%d' does not exist. Returning nullptr.", hMesh);
		return nullptr;
	}

	return &m_MeshDictionary.m_MeshArray[hMesh];
}

//-------------------------------------------------------------------------
// Purpose: Releases a reference to a mesh, decrementing it's reference count.
// 
// Input:	hMesh - The mesh to release an instance from.
// Output:	none.
//-------------------------------------------------------------------------
void CMeshSystem::ReleaseMeshInstance(MeshHandle_t hMesh)
{
	CAssert(hMesh != kInvalidID_U32);

	Mesh*		pMesh = &m_MeshDictionary.m_MeshArray[hMesh];
	MeshLookup* pLookup = &m_MeshDictionary.m_MeshHashmap[pMesh->m_pszMeshPath];

	// Decrement the reference count.
	pLookup->iReferenceCount--;
	CAssert(pLookup->iReferenceCount >= 0);

	if (pLookup->iReferenceCount == 0 && pLookup->bAutoRelease)
	{
		// Time to release the mesh.
		LOG_VERBOSE("Releasing mesh '%s'.", pMesh->m_pszMeshPath);
		
		for (int iSubmeshIdx = 0; iSubmeshIdx < pMesh->m_Geometries.Count(); iSubmeshIdx++)
		{
			g_pRenderer->ReleaseGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, pMesh->m_Geometries[iSubmeshIdx]);
			g_pMaterialSystemInternal->ReleaseMaterial(pMesh->m_Geometries[iSubmeshIdx].hMaterial, false);
		}
		ClearMeshDictionaryEntry(pMesh->m_pszMeshPath);
	}
}

//-------------------------------------------------------------------------
// Purpose: Find the first available handle in the mesh dictionary.
// Input:	none.
// Output:	Returns the first available mesh handle.
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::GetFirstFreeMeshHandle() const
{
	if (!s_MeshSystem.m_MeshDictionary.m_MeshArray)
	{
		LOG_ERROR("Attempting to acquire a mesh prior to mesh system initialization, mesh acquisition failed.");
		return kInvalidID_U32;
	}

	for (int meshIdx = 0; meshIdx < m_Config.MaxMeshes; meshIdx++)
	{
		if (s_MeshSystem.m_MeshDictionary.m_MeshArray[meshIdx].m_MeshID == kInvalidID_U32)
		{
			// Found a free slot, return!
			return meshIdx;
		}
	}

	LOG_WARN("Failed to find free mesh slot in the mesh array, returning null handle.");
	return kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Clears a given mesh dictionary entry.
// Input:	pszMeshPath - The path of the mesh whose entry to vacate.
// Output:	none.
//-------------------------------------------------------------------------
void CMeshSystem::ClearMeshDictionaryEntry(const char* pszMeshPath)
{
	CAssert(pszMeshPath);

	MeshLookup* pLookup = &m_MeshDictionary.m_MeshHashmap[pszMeshPath];
	Mesh*		pMesh = &m_MeshDictionary.m_MeshArray[pLookup->MeshHandle];

	CAssert(pLookup && pMesh);

	pMesh->m_Geometries.Clear();
	pMesh->m_matLocalToWorldTransform = Matrix4x4{ 1.0f };
	pMesh->m_MeshID = kInvalidID_U32;
	pMesh->m_vecAngles = pMesh->m_vecPosition = pMesh->m_vecScale = Vector3D{ 0.0f, 0.0f, 0.0f };
	pMesh->vecMaxExtents = pMesh->vecMinExtents = Vector3D{ 0.0f, 0.0f, 0.0f };
	
	Utl::StringFree(pMesh->m_pszMeshPath);
	pMesh->m_pszMeshPath = nullptr;

	pLookup->bAutoRelease = false;
	pLookup->iReferenceCount = 0;
	pLookup->MeshHandle = kInvalidID_U32;
}


