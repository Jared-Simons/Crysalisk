//=============================================================================
// Author: Jared Simons
// Date Created: 12/10/2025
// 
// Purpose: The mesh system.
//=============================================================================
#include "MeshSystem.h"

#include <Containers/CHashMap.h>
#include "Systems/JobSystem.h"
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Purpose: MESHSYSTEMTODO: Documentation.
//-------------------------------------------------------------------------
class CMeshSystem : public IMeshSystem
{
public:
	virtual bool			Initialize(const MeshSystemConfig_t& pConfig) override;
	virtual void			Shutdown() override;

	virtual MeshHandle_t	AcquireMesh(const char* pszMeshName, bool bAutoRelease) override;

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

static CMeshSystem s_MeshSystem;
IMeshSystem* g_pMeshSystem = &s_MeshSystem;

static int OnMeshLoadJob_Success(JobResults_t& pResults)
{
	MeshLoader::MeshLoaderParams_t* pTypedResults = reinterpret_cast<MeshLoader::MeshLoaderParams_t*>(pResults.pUserData);

	MeshLoader::MeshResource_t::GeometryGroup_t*	pMesh = &pTypedResults->pOutResource->pObjects[0];
	MeshLoader::MeshResource_t::GeometryObject_t*	pSubmesh = nullptr;

	for (int iSubMeshIdx = 0; iSubMeshIdx < pMesh->pGroups.Capacity(); iSubMeshIdx++)
	{
		pSubmesh = &pMesh->pGroups[iSubMeshIdx];
		CAssert(pSubmesh != nullptr);

		GeometryConfig_t newConfig{};
		newConfig.Vertices = pSubmesh->Vertices;
		newConfig.Indices = pSubmesh->Indices;
		if (pSubmesh->pszDebugName)
			newConfig.pszDebugName = Utl::StringDuplicate(pSubmesh->pszDebugName);

		if (newConfig.Vertices.Count() > 0)
		{
			LOG_VERBOSE("Acquiring geometry resources for sub-mesh '%s'.", pSubmesh->pszDebugName);

			// Leftoff, write this data to the new mesh entry.
		}
	}

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
// Purpose: MESHSYSTEMTODO: Documentation.
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
// Purpose: MESHSYSTEMTODO: Documentation.
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
// Purpose: MESHSYSTEMTODO: Documentation.
//-------------------------------------------------------------------------
MeshHandle_t CMeshSystem::AcquireMesh(const char* pszMeshName, bool bAutoRelease)
{
	if (!pszMeshName)
	{
		LOG_ERROR("CMeshSystem::AcquireMesh requires the name of a mesh to load. Nothing was done.");
		return kInvalidID_U32;
	}

	MeshLoader::MeshResource_t* pMeshResource = new MeshLoader::MeshResource_t;
	MeshLoader::MeshLoaderParams_t* pMeshParams = new MeshLoader::MeshLoaderParams_t;

	// Format the string with the path to the model directory.
	char szFormattedPath[512]{};
	Utl::StringFormat_V("Assets/Models/%s\0", szFormattedPath, pszMeshName);

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

	return 0;
}


