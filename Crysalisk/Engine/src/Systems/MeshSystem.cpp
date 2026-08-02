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
#include "Systems/ShaderSystem.h"
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Purpose: Mesh System Implementation.
//-------------------------------------------------------------------------
class CMeshSystem : public IMeshSystem
{
public:
	virtual bool			Initialize(const MeshSystemConfig_t& pAppConfig) override;
	virtual void			Shutdown() override;

	virtual MeshHandle_t	AcquireMeshBase(const char* pszMeshName) override;
	virtual u32				AcquireMeshInstance(MeshHandle_t hBaseMesh, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) override;
	virtual MeshInstance_t* AcquireMeshInstanceDataFromHandle(u32 hMeshInstance) const override;
	virtual MeshBase_t*		AcquireMeshBaseDataFromHandle(MeshHandle_t hMesh) const override;

	virtual void			DrawGeometry(u32 hMeshInstance, class IShaderSystem* pShaderSystem) const override;
	virtual void			DrawGeometryBySubmesh(u32 hMeshInstance, u32 iSubmesh, class IShaderSystem* pShaderSystem) const override;

	virtual MeshHandle_t	AcquireMeshInstance(const char* pszMeshName, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease) override;
	virtual MeshHandle_t	AcquireMeshFromResource(MeshLoader::MeshResource_t& pResource, const char* pszMeshName, bool bAutoRelease) override;
	virtual Mesh*			AcquireMeshDataByHandle(MeshHandle_t hMesh) override;

	virtual MeshHandle_t	WrapGeometryInstanceInMesh(GeometryInstance_t* pGeometryInstance, const char* pszName, MaterialHandle_t hMaterial) override;

	virtual void			ReleaseMeshInstance(MeshHandle_t hMesh) override;

	void					AcquireMeshBaseFromResource(MeshLoader::MeshLoaderParams_t* pResource);

private:
	// FIXME: Deprecate this system and move to mesh bases/instances.
	MeshHandle_t			GetFirstFreeMeshHandle() const;

	u32						GetFirstFreeMeshBaseIdx() const;
	u32						GetFirstFreeMeshInstanceIdx() const;

	void					ClearMeshDictionaryEntry(const char* pszMeshPath);

private:
	MeshSystemConfig_t	m_Config{};

	RenderBuffer*		m_pWorldVertexBuffer{ nullptr };
	RenderBuffer*		m_pWorldIndexBuffer{ nullptr };

	struct MeshLookup
	{
		MeshHandle_t	MeshHandle{ kInvalidID_U32 };
		i32				iReferenceCount{ 0 };
		bool			bAutoRelease{ false };
	};

	struct MeshDictionary_t
	{
		Utl::CStringHashMap<MeshLookup>	m_MeshHashmap{};
		Mesh*							m_MeshArray{ nullptr };
	};

	// The mesh dictionary.
	// FIXME: Deprecate.
	MeshDictionary_t	m_MeshDictionary{};

	// An array of base meshes.
	MeshBase_t*			m_MeshBases{ nullptr };

	// An array of instances.
	MeshInstance_t*		m_MeshInstances{ nullptr };

	// String->u32 lookup table.
	Utl::CStringHashMap<MeshLookup>	m_MeshLookup{};

	// An array of transform matrices.
	Matrix4x4*			m_Transforms{ nullptr };

	// Instance counts
	// FIXME: How to handle different instances?
	u32					m_RunningInstanceTotal{ 0 };
	u32*				m_MeshInstanceCounts{ nullptr };
};

// Mesh System Context
static CMeshSystem* s_MeshSystem{ nullptr };

//-------------------------------------------------------------------------
// Purpose: Invoked upon successful completion of an async mesh-loading job.
// TODO: Deprecate.
//-------------------------------------------------------------------------
static i32 OnMeshLoadJob_Success(JobResults_t& pResults)
{
	// LEFTOFF: Make this work with the new mesh format.
	MeshLoader::MeshLoaderParams_t* pTypedResults = reinterpret_cast<MeshLoader::MeshLoaderParams_t*>(pResults.pUserData);

	// This will handle mesh registration and GPU upload.
	s_MeshSystem->AcquireMeshFromResource(*pTypedResults->pOutResource, pTypedResults->pszMeshPath, true);

	// Release all the resources allocated for this job.
	// TODO: Make sure this is properly releasing EVERYTHING
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
// Purpose: Invoked upon successful completion of an async mesh-loading job.
//-------------------------------------------------------------------------
static i32 OnMeshLoadJob_Success_NEW(JobResults_t& pResults)
{
	MeshLoader::MeshLoaderParams_t* pTypedResults = reinterpret_cast<MeshLoader::MeshLoaderParams_t*>(pResults.pUserData);

	// This will handle mesh registration and GPU upload.
	s_MeshSystem->AcquireMeshBaseFromResource(pTypedResults);

	// Release all the resources allocated for this job.
	// TODO: Make sure this is properly releasing EVERYTHING
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
// Purpose: Invoked upon mesh loading job failed.
//-------------------------------------------------------------------------
static i32 OnMeshLoadJob_Fail(JobResults_t& pResults)
{
	MeshLoader::MeshLoaderParams_t* pTypedResults = reinterpret_cast<MeshLoader::MeshLoaderParams_t*>(pResults.pUserData);

	if (!pTypedResults)
	{
		return 0;
	}

	LOG_ERROR("Failed to load mesh resource '%s'.", pTypedResults->pszMeshPath);

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
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IMeshSystem::AllocateMeshSystemState(IMeshSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold mesh system state was not provided! Mesh system creation failed!");
		return;
	}

	s_MeshSystem = new CMeshSystem;
	*ppSystemState = s_MeshSystem;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMeshSystem::Initialize(const MeshSystemConfig_t& pAppConfig)
{
	if (pAppConfig.MaxMeshes < 1)
	{
		LOG_ERROR("Mesh System REQUIRES Config.MeshMeshes > 0.");
		return false;
	}

	m_Config = pAppConfig;

	m_MeshDictionary.m_MeshArray = new Mesh[m_Config.MaxMeshes];
	m_MeshDictionary.m_MeshHashmap.Resize(m_Config.MaxMeshes);

	m_MeshBases = new MeshBase_t[m_Config.MaxMeshes];
	m_MeshInstances = new MeshInstance_t[m_Config.MaxMeshes];
	m_MeshLookup.Resize(m_Config.MaxMeshes);
	m_Transforms = new Matrix4x4[m_Config.MaxMeshes];

	m_MeshInstanceCounts = new u32[m_Config.MaxMeshes];

	// Get geometry buffers
	m_pWorldVertexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(Vertex3D) * 5 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "World_Vertex_Buffer");
	if (m_pWorldVertexBuffer == nullptr)
	{
		LOG_ERROR("Failed to create world geometry vertex buffer!");
		return false;
	}

	m_pWorldIndexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(u32) * 5 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "World_Index_Buffer");
	if (m_pWorldIndexBuffer == nullptr)
	{
		LOG_ERROR("Failed to create world geometry index buffer!");
		return false;
	}

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

	if (m_Transforms)
	{
		delete[] m_Transforms;
		m_Transforms = nullptr;
	}

	if (m_MeshInstances)
	{
		delete[] m_MeshInstances;
		m_MeshInstances = nullptr;
	}

	if (m_MeshBases)
	{
		delete[] m_MeshBases;
		m_MeshBases = nullptr;
	}

	if (m_MeshInstanceCounts)
	{
		delete[] m_MeshInstanceCounts;
		m_MeshInstanceCounts = nullptr;
	}

	s_MeshSystem = nullptr;
}

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::AcquireMeshBase(const char* pszMeshName)
{
	if (!pszMeshName)
	{
		LOG_ERROR("CMeshSystem::AcquireMeshBase requires the name of a mesh to load. Nothing was done.");
		return kInvalidID_U32;
	}

	MeshLookup* pLookup = m_MeshLookup.Insert(pszMeshName);
	MeshBase_t* pBase = nullptr;
	MeshHandle_t hMeshBase = kInvalidID_U32;

	if (pLookup->MeshHandle == kInvalidID_U32)
	{
		// First time with this mesh, need to load resources.
		hMeshBase = GetFirstFreeMeshBaseIdx();
		pBase = &m_MeshBases[hMeshBase];

		CASSERTMSG(pBase->iMeshIdx == kInvalidID_U32, "AcquireMeshBase() - Got an occupied base mesh handle!");

		pBase->iMeshIdx = hMeshBase;
		pLookup->MeshHandle = hMeshBase;

		pBase->pszName = Utl::StringDuplicate(pszMeshName);
		
		// Allocate a new mesh resource for loading.
		MeshLoader::MeshResource_t* pMeshResource = new MeshLoader::MeshResource_t;
		MeshLoader::MeshLoaderParams_t* pMeshParams = new MeshLoader::MeshLoaderParams_t;

		pMeshParams->pszMeshPath = Utl::StringDuplicate(pszMeshName);
		pMeshParams->pOutResource = pMeshResource;

		// Queue up a job to load the mesh.
		JobDeclaration_t meshLoadJob{};
		meshLoadJob.pfnEntryPoint = MeshLoader::LoadMesh_Threaded;
		meshLoadJob.pParams = pMeshParams;
		meshLoadJob.iParamSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.pJobResults.pfnOnJobCompleted_Success_Callback = OnMeshLoadJob_Success_NEW;
		meshLoadJob.pJobResults.pfnOnJobCompleted_Failed_Callback = OnMeshLoadJob_Fail;
		meshLoadJob.pJobResults.iUserDataSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.iJobType = WORKER_THREAD_TYPE_DISK_LOADING;
		meshLoadJob.ePriority = eJobPriority::NORMAL;

		m_Config.pJobSystem->SubmitJob(meshLoadJob);
	}
	else
	{
		// Mesh resources are already loaded.
		hMeshBase = pLookup->MeshHandle;
	}

	return hMeshBase;
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

	pMeshLookup = m_MeshDictionary.m_MeshHashmap.Insert(pszMeshName);

	if (pMeshLookup->MeshHandle == kInvalidID_U32)
	{
		// The mesh hasn't been loaded yet. Load it up!
		newMeshHandle = GetFirstFreeMeshHandle();
		if (newMeshHandle == kInvalidID_U32)
		{
			LOG_ERROR("AcquireMeshInstance() could not acquire a new mesh slot. Increase mesh system config to allow for more. Returning null handle.");
			return kInvalidID_U32;
		}

		MeshLoader::MeshResource_t* pMeshResource = new MeshLoader::MeshResource_t;
		MeshLoader::MeshLoaderParams_t* pMeshParams = new MeshLoader::MeshLoaderParams_t;

		pMesh = &m_MeshDictionary.m_MeshArray[newMeshHandle];
		pMesh->m_MeshID = newMeshHandle;

		pMesh->m_pszMeshPath = Utl::StringDuplicate(pszMeshName);

		pMesh->m_vecPosition = vecPos;
		pMesh->m_vecAngles = vecEulerAngles;
		pMesh->m_vecScale = vecScale;

		pMeshLookup->MeshHandle = newMeshHandle;

		pMeshParams->pszMeshPath = Utl::StringDuplicate(pszMeshName);
		pMeshParams->pOutResource = pMeshResource;

		JobDeclaration_t meshLoadJob{};
		meshLoadJob.pfnEntryPoint = MeshLoader::LoadMesh_Threaded;
		meshLoadJob.pParams = pMeshParams;
		meshLoadJob.iParamSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.pJobResults.pfnOnJobCompleted_Success_Callback = OnMeshLoadJob_Success;
		meshLoadJob.pJobResults.pfnOnJobCompleted_Failed_Callback = OnMeshLoadJob_Fail;
		meshLoadJob.pJobResults.iUserDataSize = sizeof(MeshLoader::MeshLoaderParams_t);
		meshLoadJob.iJobType = WORKER_THREAD_TYPE_DISK_LOADING;
		meshLoadJob.ePriority = eJobPriority::NORMAL;

		m_Config.pJobSystem->SubmitJob(meshLoadJob);
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
// Purpose: TODO: Documentation.
//-------------------------------------------------------------------------
u32 CMeshSystem::AcquireMeshInstance(MeshHandle_t hBaseMesh, Vector vecPos, Vector vecEulerAngles, Vector vecScale, bool bAutoRelease)
{
	CAssert(hBaseMesh != kInvalidID_U32 && m_MeshBases[hBaseMesh].iMeshIdx != kInvalidID_U32);

	u32 instanceIdx = GetFirstFreeMeshInstanceIdx();

	MeshInstance_t* instance = &m_MeshInstances[instanceIdx];
	instance->hBaseMesh = hBaseMesh;
	instance->iInstanceIdx = m_MeshInstanceCounts[hBaseMesh];
	m_MeshInstanceCounts[hBaseMesh]++;

	instance->iInstanceIdx = m_RunningInstanceTotal;
	m_RunningInstanceTotal++;

	Matrix4x4 matLocalToWorld(1.0f);
	matLocalToWorld = Matrix4x4::Scale(matLocalToWorld, vecScale);
	
	// FIXME: Implement euler rotation.
	matLocalToWorld = Matrix4x4::Translate(matLocalToWorld, vecPos);
	instance->matLocalToWorld = matLocalToWorld;

	return instanceIdx;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
MeshInstance_t* CMeshSystem::AcquireMeshInstanceDataFromHandle(u32 hMeshInstance) const
{
	CASSERTMSG(hMeshInstance != kInvalidID_U32, "AcquireMeshInstanceDataFromHandle requires a valid mesh handle!");

	return &m_MeshInstances[hMeshInstance];
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CMeshSystem::DrawGeometry(u32 hMeshInstance, class IShaderSystem* pShaderSystem) const
{
	CAssert(hMeshInstance != kInvalidID_U32 && pShaderSystem);

	MeshInstance_t* pInstance = &m_MeshInstances[hMeshInstance];
	MeshBase_t* pBase = &m_MeshBases[pInstance->hBaseMesh];

	// Bind the model matrix.
	m_Config.pMaterialSystem->ApplyPerDrawRenderData(pInstance->matLocalToWorld);

	for (u32 iMesh = 0; iMesh < pBase->renderData.Count(); iMesh++)
	{
		// Bind the material.
		m_Config.pMaterialSystem->ApplyMaterialInstance(pBase->renderData[iMesh].hMaterial);

		// Draw!
		m_Config.pRenderer->DrawGeometry(&pBase->renderData[iMesh], m_pWorldVertexBuffer, m_pWorldIndexBuffer);
	}
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CMeshSystem::DrawGeometryBySubmesh(u32 hMeshInstance, u32 iSubmesh, class IShaderSystem* pShaderSystem) const
{
	CAssert(hMeshInstance != kInvalidID_U32 && iSubmesh != kInvalidID_U32 && pShaderSystem);

	MeshInstance_t* pInstance = &m_MeshInstances[hMeshInstance];
	MeshBase_t* pBase = &m_MeshBases[pInstance->hBaseMesh];

	// Bind the model matrix.
	m_Config.pMaterialSystem->ApplyPerDrawRenderData(pInstance->matLocalToWorld);

	// Bind the material.
	m_Config.pMaterialSystem->ApplyMaterialInstance(pBase->renderData[iSubmesh].hMaterial);

	// Draw!
	m_Config.pRenderer->DrawGeometry(&pBase->renderData[iSubmesh], m_pWorldVertexBuffer, m_pWorldIndexBuffer);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
MeshBase_t* CMeshSystem::AcquireMeshBaseDataFromHandle(MeshHandle_t hMesh) const
{
	CASSERTMSG(hMesh != kInvalidID_U32, "AcquireMeshBaseDataFromHandle requires a valid mesh handle!");

	return &m_MeshBases[hMesh];
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CMeshSystem::AcquireMeshBaseFromResource(MeshLoader::MeshLoaderParams_t* pResource)
{
	MeshLookup*		pMeshLookup = &m_MeshLookup[pResource->pszMeshPath];
	CAssert(pMeshLookup->MeshHandle != kInvalidID_U32);

	MeshBase_t*		pMeshBase = &m_MeshBases[pMeshLookup->MeshHandle];

	// Take a convenience pointer to the mesh resources.
	Utl::CVector<MeshLoader::MeshResource_t::GeometryObject_t>* pMeshResources = &pResource->pOutResource->pMeshes;
	Utl::CVector<char*>*										pMaterialNameTable = &pResource->pOutResource->pMaterialNames;
	
	// Acquire renderer resources and upload to the GPU.
	pMeshBase->renderData.Resize(pMeshResources->Count());
	pMeshBase->renderData.SetCount(pMeshResources->Count());

	pMeshBase->submeshMinExtents.Resize(pMeshResources->Count());
	pMeshBase->submeshMaxExtents.Resize(pMeshResources->Count());
	for (u32 iMesh = 0; iMesh < pMeshResources->Count(); iMesh++)
	{
		MeshRenderData_t* pRenderData = &pMeshBase->renderData[iMesh];
		MeshLoader::MeshResource_t::GeometryObject_t* pCurMesh = &pMeshResources->ElementAt(iMesh);

		// Do the upload.
		m_Config.pRenderer->AcquireGeometryResources(*m_pWorldVertexBuffer, *m_pWorldIndexBuffer, pCurMesh->iVertexCount, pCurMesh->iVertexStride, pCurMesh->iVertexOffset, pCurMesh->pVertexData, pCurMesh->iIndexCount, pCurMesh->iIndexData, pRenderData);

		// Get material.
		if (pMaterialNameTable->Count() > 0 && pResource->pOutResource->pMeshes[iMesh].iMaterialNameIndex != kInvalidID_U32)
		{
			pRenderData->hMaterial = m_Config.pMaterialSystem->AcquireMaterial(pMaterialNameTable->ElementAt(pCurMesh->iMaterialNameIndex));
		}

		pMeshBase->submeshMinExtents.PushBack(pCurMesh->vecMinExtents);
		pMeshBase->submeshMaxExtents.PushBack(pCurMesh->vecMaxExtents);
		pMeshBase->submeshCenters.PushBack((pCurMesh->vecMaxExtents + pCurMesh->vecMinExtents) * 0.5f);
	}

	// LEFTOFF: Overall mesh extents

	// Clean up mesh resources.
	MeshLoader::UnloadMeshResource(*pResource->pOutResource);
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

	// Acquire renderer resources and upload to the GPU.
	for (i32 iSubmesh = 0; iSubmesh < pResource.pMeshes.Count(); iSubmesh++)
	{
		MeshLoader::MeshResource_t::GeometryObject_t& pCurSubmesh = pResource.pMeshes[iSubmesh];

		pMesh->m_Geometries.PushBack(GeometryInstance_t{});

		GeometryInstance_t& pNewSubmesh = pMesh->m_Geometries.Back();

		// Acquire material resources.
		if (pResource.pMaterialNames.Count() > 0 && pResource.pMeshes[iSubmesh].iMaterialNameIndex != kInvalidID_U32)
		{
			pNewSubmesh.hShaderInstance = m_Config.pMaterialSystem->AcquireMaterial(pResource.pMaterialNames[pResource.pMeshes[iSubmesh].iMaterialNameIndex]);
		}
		else
		{
			// Just grab the default if no materials were provided.
			pNewSubmesh.hShaderInstance = m_Config.pMaterialSystem->GetDefaultMaterialReference();
		}

		pNewSubmesh.vertexDataOffset = pCurSubmesh.iVertexOffset;
		pNewSubmesh.indexDataOffset = pCurSubmesh.iIndexOffset;
		pNewSubmesh.vecMinExtents = pCurSubmesh.vecMinExtents;
		pNewSubmesh.vecMaxExtents = pCurSubmesh.vecMaxExtents;
		pNewSubmesh.vecCenter = (pNewSubmesh.vecMinExtents + pNewSubmesh.vecMaxExtents) * 0.5f;
		m_Config.pRenderer->AcquireGeometryResources(*m_pWorldVertexBuffer, *m_pWorldIndexBuffer,
													pCurSubmesh.iVertexCount, pCurSubmesh.iVertexStride, pCurSubmesh.iVertexOffset, pCurSubmesh.pVertexData,
													pCurSubmesh.iIndexCount, pCurSubmesh.iIndexData, &pNewSubmesh);
	}

	// Increment the reference count.
	pMeshLookup->iReferenceCount++;
	pMeshLookup->bAutoRelease = bAutoRelease;

	// Clean up mesh resources.
	// NOTE: Put this on a thread?
	MeshLoader::UnloadMeshResource(pResource);

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
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::WrapGeometryInstanceInMesh(GeometryInstance_t* pGeometryInstance, const char* pszName, MaterialHandle_t hMaterial)
{
	CASSERTMSG(pGeometryInstance && pszName, "WrapGeometryInstanceInMesh requires a pointer to geometry and name.");

	MeshLookup* pLookup = m_MeshDictionary.m_MeshHashmap.Insert(pszName);
	u32 meshBaseIdx = GetFirstFreeMeshBaseIdx();

	MeshBase_t* pBase = &m_MeshBases[meshBaseIdx];

	pBase->iMeshIdx = meshBaseIdx;
	pBase->pszName = Utl::StringDuplicate(pszName);
	pBase->geometry = *pGeometryInstance;

	// FIXME: Remove GeometryInstance_t
	{
		pBase->renderData.Resize(1);
		pBase->renderData.SetCount(1);
		pBase->renderData[0].vertexCount = pGeometryInstance->vertexCount;
		pBase->renderData[0].vertexStride = pGeometryInstance->vertexDataStride;
		pBase->renderData[0].vertexBufferOffset = pGeometryInstance->vertexDataOffset;
		pBase->renderData[0].indexCount = pGeometryInstance->indexCount;
		pBase->renderData[0].indexStride = pGeometryInstance->indexDataStride;
		pBase->renderData[0].indexDataOffset = pGeometryInstance->indexDataOffset;
		pBase->renderData[0].hMaterial = hMaterial;
	}

	pLookup->MeshHandle = meshBaseIdx;

	return meshBaseIdx;
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

	if (pMesh->m_Geometries.Count() < 1)
	{
		// It's possible that the application is trying to release this mesh before it has even loaded.
		// (e.g. it's shutting down).
		// Just abort here.
		return;
	}

	// Decrement the reference count.
	pLookup->iReferenceCount--;
	CAssert(pLookup->iReferenceCount >= 0);

	if (pLookup->iReferenceCount == 0 && pLookup->bAutoRelease)
	{
		// Time to release the mesh.
		LOG_VERBOSE("Releasing mesh '%s'.", pMesh->m_pszMeshPath);
		
		for (i32 iSubmeshIdx = 0; iSubmeshIdx < pMesh->m_Geometries.Count(); iSubmeshIdx++)
		{
			// TODO: Fix this
			//m_Config.pRenderer->ReleaseGeometryResources(m_Config.pRenderer->GetWorldVertexBuffer(), m_Config.pRenderer->GetWorldIndexBuffer(), pMesh->m_Geometries[iSubmeshIdx]);
			m_Config.pMaterialSystem->ReleaseMaterial(pMesh->m_Geometries[iSubmeshIdx].hShaderInstance, false);
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
	if (!s_MeshSystem->m_MeshDictionary.m_MeshArray)
	{
		LOG_ERROR("Attempting to acquire a mesh prior to mesh system initialization, mesh acquisition failed.");
		return kInvalidID_U32;
	}

	for (i32 meshIdx = 0; meshIdx < m_Config.MaxMeshes; meshIdx++)
	{
		if (s_MeshSystem->m_MeshDictionary.m_MeshArray[meshIdx].m_MeshID == kInvalidID_U32)
		{
			// Found a free slot, return!
			return meshIdx;
		}
	}

	LOG_WARN("Failed to find free mesh slot in the mesh array, returning null handle.");
	return kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available base mesh slot in the mesh dictionary.
// Input:	none.
// Output:	Returns the first available base mesh index.
//-------------------------------------------------------------------------
u32 CMeshSystem::GetFirstFreeMeshBaseIdx() const
{
	for (u32 meshIdx = 0; meshIdx < m_Config.MaxMeshes; meshIdx++)
	{
		if (s_MeshSystem->m_MeshBases[meshIdx].iMeshIdx == kInvalidID_U32)
		{
			// Found a free slot, return!
			return meshIdx;
		}
	}

	LOG_WARN("Failed to find free base mesh slot in the mesh array, returning null handle.");
	return kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available instance mesh slot in the mesh dictionary.
// Input:	none.
// Output:	Returns the first available base mesh index.
//-------------------------------------------------------------------------
u32 CMeshSystem::GetFirstFreeMeshInstanceIdx() const
{
	for (u32 meshIdx = 0; meshIdx < m_Config.MaxMeshes; meshIdx++)
	{
		if (s_MeshSystem->m_MeshInstances[meshIdx].hBaseMesh == kInvalidID_U32)
		{
			// Found a free slot, return!
			return meshIdx;
		}
	}

	LOG_WARN("Failed to find free instance mesh slot in the mesh array, returning null handle.");
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