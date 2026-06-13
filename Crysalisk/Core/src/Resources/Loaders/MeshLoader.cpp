//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#include "MeshLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"
#include "Resources/Loaders/MaterialLoader.h"
#include "Resources/Serializers/MaterialSerializer.h"
#include "Resources/Serializers/MeshSerializer.h"
#include "Utils/Utils.h"

#include <stdio.h> // For sscanf()

namespace MeshLoader
{
	// Helper functions.
	static bool LoadCModelMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource);

	//-------------------------------------------------------------------------
	// Purpose: The entrypoint for MeshLoader jobs.
	// 
	// Input:	pParams - An opaque pointer to be cast to an MeshLoaderParams_t struct.
	//			pJobResults - An opaque pointer that will hold the results of this job. 
	//				(In this case, the results are held in the same struct as the params).
	// Output:	int - 0 indicates the job failed, 1 indicates the job completed successfully.
	//-------------------------------------------------------------------------
	int LoadMesh_Threaded(void* pParams, void** pJobResults)
	{
		if (!pParams)
		{
			LOG_ERROR("LoadMesh_Threaded was provided with no parameters. Mesh load failed.");
			return 0;
		}

		// Job results for this job are the same as the params.
		*pJobResults = pParams;

		MeshLoaderParams_t* pTypedParams = reinterpret_cast<MeshLoaderParams_t*>(pParams);

		if (!LoadMeshResource(pTypedParams->pszMeshPath, *pTypedParams->pOutResource))
		{
			return 0;
		}

		LOG_VERBOSE("Successfully loaded mesh '%s'.", pTypedParams->pszMeshPath);
		return 1;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing mesh asset from disk.
	// 
	// Input:	pszMeshFileName - The path to the image to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadMeshResource(char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		// Check .cmodel_c files first.
		char szFormattedPathWithExtension[512]{};
		Utl::StringFormat_V("%s%s", szFormattedPathWithExtension, pszMeshFileName, ".cmodel_c");

		if (!FileSystem::FileExists(szFormattedPathWithExtension))
		{
			// Check it with other supported formats.
			Utl::StringFormat_V("%s%s", szFormattedPathWithExtension, pszMeshFileName, ".obj");
			if (FileSystem::FileExists(szFormattedPathWithExtension))
			{
				return MeshSerializer::SerializeOBJMesh(szFormattedPathWithExtension, pszMeshFileName, pOutResource);
			}

			LOG_ERROR("Mesh resource file '%s' could not be found. Mesh resource load failed.", pszMeshFileName);
			return false;
		}
		else
		{
			return LoadCModelMeshResource(szFormattedPathWithExtension, pOutResource);
		}

		CAssert(0);
		return false;
	}

	//-------------------------------------------------------------------------
	// Purpose: Unloads mesh resource data.
	// 
	// Input:	pResource - The mesh resource to be unloaded.
	// Output:	none.
	//-------------------------------------------------------------------------
	void UnloadMeshResource(MeshResource_t& pResource)
	{
		for (uint64_t iMesh = 0; iMesh < pResource.pMeshes.Count(); iMesh++)
		{
			if (pResource.pMeshes[iMesh].pVertexData)
			{
				delete[] pResource.pMeshes[iMesh].pVertexData;
				pResource.pMeshes[iMesh].pVertexData = nullptr;
			}
			if (pResource.pMeshes[iMesh].iIndexData)
			{
				delete[] pResource.pMeshes[iMesh].iIndexData;
				pResource.pMeshes[iMesh].iIndexData = nullptr;
			}
		}

		for (uint64_t iMat = 0; iMat < pResource.pMaterialNames.Count(); iMat++)
		{
			Utl::StringFree(pResource.pMaterialNames[iMat]);
			pResource.pMaterialNames[iMat] = nullptr;
		}
	}
	
	//-------------------------------------------------------------------------
	// Purpose: Attempts to a Crysalisk model (CModel) at the given path.
	// 
	// Input:	pszMeshFileName - The path to the model to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	static bool LoadCModelMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		Utl::CVector<uint8_t> modelBytes{};
		FileHandle_t hCModelFile;
		size_t modelDataSize{ 0 };
		size_t bytesRead{ 0 };

		if (!FileSystem::OpenFile(pszMeshFileName, eFileMode::FILEMODE_READ, true, &hCModelFile))
		{
			return false;
		}

		modelDataSize = FileSystem::FileSize(&hCModelFile);
		modelBytes.Resize(modelDataSize);
		FileSystem::ReadWholeBinaryFile(modelBytes.Data(), modelDataSize, &bytesRead, &hCModelFile);

		void* pDataPtr = modelBytes.Data();
		uint64_t	currOffset{ 0 };

		// First up is the magic number.
		uint32_t* pMagicNumber = (uint32_t*)modelBytes.Data();
		if (*pMagicNumber != MeshSerializer::kModelResourceMagicNumber)
		{
			LOG_ERROR("'%s' mesh does not contain kModelResourceMagicNumber, resource reading failed.", pszMeshFileName);
			return false;
		}

		// Advance to the header.
		currOffset += sizeof(uint32_t);

		MeshSerializer::CModel_FileHeader_t* pFileHeader = (MeshSerializer::CModel_FileHeader_t*)((uint8_t*)pDataPtr + currOffset);

		currOffset += sizeof(MeshSerializer::CModel_FileHeader_t);
		byte* pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		CAssert(*(uint32_t*)pBytes == MeshSerializer::kModelResourceSubmeshHeader);
		if (*(uint32_t*)pBytes != MeshSerializer::kModelResourceSubmeshHeader)
		{
			LOG_ERROR("Mesh '%s' missing expected Submesh Header marker. Please recompile.", pszMeshFileName);
		}

		currOffset += sizeof(uint32_t);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		MeshSerializer::CModel_MeshHeader_t* pMeshHeaders = new MeshSerializer::CModel_MeshHeader_t[pFileHeader->iMeshDataCount];

		pOutResource.pMeshes.Resize(pFileHeader->iMeshDataCount);
		pOutResource.pMeshes.SetCount(pFileHeader->iMeshDataCount);
		
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			Platform::MemoryCopy(&pMeshHeaders[iMesh], (MeshSerializer::CModel_MeshHeader_t*)pBytes + iMesh, sizeof(MeshSerializer::CModel_MeshHeader_t));
			pOutResource.pMeshes[iMesh].iMaterialNameIndex = pMeshHeaders[iMesh].iMaterialNameIndex;
			pOutResource.pMeshes[iMesh].vecMinExtents = pMeshHeaders[iMesh].vecMinExtents;
			pOutResource.pMeshes[iMesh].vecMaxExtents = pMeshHeaders[iMesh].vecMaxExtents;
			pOutResource.pMeshes[iMesh].iVertexStride = pFileHeader->iVertexStride;
			pOutResource.pMeshes[iMesh].iIndexStride = pFileHeader->iIndexStride;
		}

		currOffset += sizeof(MeshSerializer::CModel_MeshHeader_t) * pFileHeader->iMeshDataCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		CAssert(*(uint32_t*)pBytes == MeshSerializer::kModelResourceVertexDataHeader);
		if (*(uint32_t*)pBytes != MeshSerializer::kModelResourceVertexDataHeader)
		{
			LOG_ERROR("Mesh '%s' missing expected Vertex Header marker. Please recompile.", pszMeshFileName);
		}

		currOffset += sizeof(uint32_t);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		float* pUnpackedVertices = new float[11 * pFileHeader->iVertexCount];	// 11 floats per vertex.

		// Unpack the packed vertex data.
		uint8_t iAttribIdx = 0;
		for (uint32_t iFloat = 0; iFloat < pFileHeader->iVertexCount; iFloat++)
		{
			uint16_t* pPacked = (uint16_t*)pBytes + iFloat;

			float unpackedData = UnpackF32FromUint16(*pPacked);
			// See the comment in the corresponding switch statement in SerializeOBJMesh() to see the meaning behind this switch.
			switch (iAttribIdx)
			{
			case 0:		// Position case
					unpackedData = unpackedData * (pFileHeader->vecMaxExtents.x - pFileHeader->vecMinExtents.x) + pFileHeader->vecMinExtents.x;
					pUnpackedVertices[iFloat] = unpackedData;
					break;
			case 1:
					unpackedData = unpackedData * (pFileHeader->vecMaxExtents.y - pFileHeader->vecMinExtents.y) + pFileHeader->vecMinExtents.y;
					pUnpackedVertices[iFloat] = unpackedData;
					break;
			case 2:
					unpackedData = unpackedData * (pFileHeader->vecMaxExtents.z - pFileHeader->vecMinExtents.z) + pFileHeader->vecMinExtents.z;
					pUnpackedVertices[iFloat] = unpackedData;
					break;

			case 3:		// Normal case
			case 4:
			case 5:
				{
					unpackedData = (unpackedData * 2.0f) - 1.0f;
					pUnpackedVertices[iFloat] = unpackedData;
				}
				break;

			case 6:		// Texcoord case
			case 7:
				{
					const float flPackedTexCoordMins = -64.0f;
					const float flPackedTexCoordMaxs = 64.0f;

					unpackedData = (unpackedData) * (flPackedTexCoordMaxs - flPackedTexCoordMins) - flPackedTexCoordMaxs;
					pUnpackedVertices[iFloat] = unpackedData;
				}
				break;

			case 8:		// Tangent case
			case 9:
			case 10:
			{
				unpackedData = (unpackedData * 2.0f) - 1.0f;
				pUnpackedVertices[iFloat] = unpackedData;
			}
			break;

			default:
				CAssert(0);
				break;
			}
			iAttribIdx = (iAttribIdx + 1) % 11;
		}

		// Copy over vertex data.
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			MeshResource_t::GeometryObject_t& pCurOutMesh = pOutResource.pMeshes[iMesh];
			MeshSerializer::CModel_MeshHeader_t& pCurMeshHeader = pMeshHeaders[iMesh];

			pCurOutMesh.iVertexCount = pCurMeshHeader.iVertexCount;
			pCurOutMesh.iVertexOffset = pCurMeshHeader.iVertexOffset;
			pCurOutMesh.pVertexData = new byte[sizeof(Vertex3D) * pCurMeshHeader.iVertexCount];
			
			Vertex3D* pVertexBase = (Vertex3D*)pCurOutMesh.pVertexData;

			for (uint32_t iVert = 0; iVert < pCurMeshHeader.iVertexCount; iVert++)
			{
				Platform::MemoryCopy(pVertexBase + iVert, (Vertex3D*)pUnpackedVertices + pCurMeshHeader.iVertexOffset + iVert, sizeof(Vertex3D));
			}
		}

		currOffset += sizeof(uint16_t) * pFileHeader->iVertexCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		CAssert(*(uint32_t*)pBytes == MeshSerializer::kModelResourceIndexDataHeader);
		if (*(uint32_t*)pBytes != MeshSerializer::kModelResourceIndexDataHeader)
		{
			LOG_ERROR("Mesh '%s' missing expected Index Header marker. Please recompile.", pszMeshFileName);
		}

		currOffset += sizeof(uint32_t);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Copy over index data.
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			MeshResource_t::GeometryObject_t& pCurOutMesh = pOutResource.pMeshes[iMesh];
			MeshSerializer::CModel_MeshHeader_t& pCurMeshHeader = pMeshHeaders[iMesh];

			pCurOutMesh.iIndexCount = pCurMeshHeader.iIndexCount;
			pCurOutMesh.iIndexOffset = pCurMeshHeader.iIndexOffset;
			pCurOutMesh.iIndexData = new uint32_t[pCurOutMesh.iIndexCount];

			uint32_t* pIndexBase = (uint32_t*)pCurOutMesh.iIndexData;

			for (uint32_t idx = 0; idx < pCurMeshHeader.iIndexCount; idx++)
			{
				Platform::MemoryCopy(pIndexBase + idx, (uint8_t*)pBytes + pCurOutMesh.iIndexOffset + (4 * idx), sizeof(uint32_t));
			}
		}

		currOffset += pFileHeader->iIndexStride * pFileHeader->iIndexCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		CAssert(*(uint32_t*)pBytes == MeshSerializer::kModelResourceMaterialDataHeader);
		if (*(uint32_t*)pBytes != MeshSerializer::kModelResourceMaterialDataHeader)
		{
			LOG_ERROR("Mesh '%s' missing expected Material Header marker. Please recompile.", pszMeshFileName);
		}

		currOffset += sizeof(uint32_t);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		Utl::CVector<uint32_t> materialNameOffsets(pFileHeader->iMaterialCount);

		// Get the material indices and store 'em off!
		for (uint32_t iMat = 0; iMat < pFileHeader->iMaterialCount; iMat++)
		{
			uint32_t* pNameIdx = (uint32_t*)pBytes;
			materialNameOffsets.PushBack(*pNameIdx);

			currOffset += sizeof(uint32_t);
			pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);
		}

		CAssert(*(uint32_t*)pBytes == MeshSerializer::kModelResourceStringDataHeader);
		if (*(uint32_t*)pBytes != MeshSerializer::kModelResourceStringDataHeader)
		{
			LOG_ERROR("Mesh '%s' missing expected String Header marker. Please recompile.", pszMeshFileName);
		}

		currOffset += sizeof(uint32_t);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// We are now at the string table.
		// Copy over the string names.
		pOutResource.pMaterialNames.Resize(pFileHeader->iMaterialCount);
		pOutResource.pMaterialNames.SetCount(pFileHeader->iMaterialCount);
		for (uint32_t iMat = 0; iMat < pFileHeader->iMaterialCount; iMat++)
		{
			pOutResource.pMaterialNames[iMat] = Utl::StringDuplicate((char*)pBytes + materialNameOffsets[iMat]);
		}

		// Free memory
		delete[] pUnpackedVertices;
		pUnpackedVertices = nullptr;

		delete[] pMeshHeaders;
		pMeshHeaders = nullptr;

		return true;
	}

}	// namespace MeshLoader
