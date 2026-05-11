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
	static bool LoadCModelMeshResource_Test(const char* pszMeshFileName, MeshResource_t& pOutResource);

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
	// Purpose: Attempts to a Crysalisk model (CModel) at the given path.
	// 
	// Input:	pszMeshFileName - The path to the model to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	static bool LoadCModelMeshResource_Test(const char* pszMeshFileName, MeshResource_t& pOutResource)
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

		currOffset += sizeof(uint32_t);
		pDataPtr = (uint8_t*)modelBytes.Data() + currOffset;

		uint32_t pSubheader = *(uint32_t*)pDataPtr;
		MeshSerializer::CModel_MeshHeader_t* thisHeader = nullptr;
		uint32_t* thisIndexData = nullptr;
		int16_t* thisVertexData = nullptr;
		switch (pSubheader)
		{
		case MeshSerializer::kModelResourceSubmeshHeader:
			currOffset += sizeof(uint32_t);
			pDataPtr = (uint8_t*)modelBytes.Data() + currOffset; 
			thisHeader = (MeshSerializer::CModel_MeshHeader_t*)pDataPtr;

			currOffset += sizeof(MeshSerializer::CModel_MeshHeader_t);
			pDataPtr = (uint8_t*)modelBytes.Data() + currOffset;
			thisIndexData = new uint32_t[thisHeader->iIndexCount];
			Platform::MemoryCopy(thisIndexData, pDataPtr, sizeof(uint32_t) * thisHeader->iIndexCount);

			CAssert(0);
			break;

		default:
			break;
		}

		FileSystem::CloseFile(&hCModelFile);

		return true;
	}

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

		// Vertex setup.
		uint16_t uVertexLayout = pFileHeader->iVertexLayoutFlags;
		uint64_t uVertexStride = 0;

		if (uVertexLayout & MeshSerializer::VERTEX_LAYOUT_DATA_POSITION)
			uVertexStride += sizeof(Vector);
		if (uVertexLayout & MeshSerializer::VERTEX_LAYOUT_DATA_NORMAL)
			uVertexStride += sizeof(Vector);
		if (uVertexLayout & MeshSerializer::VERTEX_LAYOUT_DATA_TEXCOORD)
			uVertexStride += sizeof(Vector2D);
		if (uVertexLayout & MeshSerializer::VERTEX_LAYOUT_DATA_TANGENT)
			uVertexStride += sizeof(Vector);

		currOffset += sizeof(MeshSerializer::CModel_FileHeader_t);
		byte* pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);
		MeshSerializer::CModel_MeshHeader_t* pMeshHeaders = new MeshSerializer::CModel_MeshHeader_t[pFileHeader->iMeshDataCount];


		pOutResource.pMeshes.Resize(pFileHeader->iMeshDataCount);
		pOutResource.pMeshes.SetCount(pFileHeader->iMeshDataCount);
		pOutResource.iVertexLayoutFlags = pFileHeader->iVertexLayoutFlags;
		
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			Platform::MemoryCopy(&pMeshHeaders[iMesh], (MeshSerializer::CModel_MeshHeader_t*)pBytes + iMesh, sizeof(MeshSerializer::CModel_MeshHeader_t));
			pOutResource.pMeshes[iMesh].iMaterialIndex = pMeshHeaders[iMesh].iMaterialIndex;
		}

		currOffset += sizeof(MeshSerializer::CModel_MeshHeader_t) * pFileHeader->iMeshDataCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		float* pUnpackedVertices = new float[11 * pFileHeader->iTotalVertexCount];	// 11 floats per vertex.

		// Unpack the packed vertex data.
		uint8_t iAttribIdx = 0;
		for (uint32_t iFloat = 0; iFloat < pFileHeader->iTotalVertexCount; iFloat++)
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

		// FIXME: Section guard?
		// Copy over vertex data.
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			MeshResource_t::GeometryObject_t& pCurOutMesh = pOutResource.pMeshes[iMesh];
			MeshSerializer::CModel_MeshHeader_t& pCurMeshHeader = pMeshHeaders[iMesh];

			pCurOutMesh.iVertexCount = pCurMeshHeader.iVertexCount;
			pCurOutMesh.iVertexOffset = pCurMeshHeader.iVertexOffset;
			//pCurOutMesh.iVertexOffset = 0;	// FIXME: Probably not gonna use this?
			pCurOutMesh.pVertexData = new byte[sizeof(Vertex3D) * pCurMeshHeader.iVertexCount];	// FIXME: FREE THESE!
			
			Vertex3D* pVertexBase = (Vertex3D*)pCurOutMesh.pVertexData;

			for (uint32_t iVert = 0; iVert < pCurMeshHeader.iVertexCount; iVert++)
			{
				Platform::MemoryCopy(pVertexBase + iVert, (Vertex3D*)pUnpackedVertices + pCurMeshHeader.iVertexOffset + iVert, sizeof(Vertex3D));
			}
		}

		currOffset += sizeof(uint16_t) * pFileHeader->iTotalVertexCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Copy over index data.
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			MeshResource_t::GeometryObject_t& pCurOutMesh = pOutResource.pMeshes[iMesh];
			MeshSerializer::CModel_MeshHeader_t& pCurMeshHeader = pMeshHeaders[iMesh];

			pCurOutMesh.iIndexCount = pCurMeshHeader.iIndexCount;
			pCurOutMesh.iIndexOffset = pCurMeshHeader.iIndexOffset;
			pCurOutMesh.iIndexData = new uint32_t[pCurOutMesh.iIndexCount];	// FIXME: FREE THESE!

			uint32_t* pIndexBase = (uint32_t*)pCurOutMesh.iIndexData;

			for (uint32_t idx = 0; idx < pCurMeshHeader.iIndexCount; idx++)
			{
				Platform::MemoryCopy(pIndexBase + idx, (uint8_t*)pBytes + pCurOutMesh.iIndexOffset + (4 * idx), sizeof(uint32_t));
			}
		}

		currOffset += pFileHeader->iTotalIndexStride * pFileHeader->iTotalIndexCount;
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Copy over material names.
		pOutResource.pMaterialNames.Resize(pFileHeader->iMaterialCount);
		pOutResource.pMaterialNames.SetCount(pFileHeader->iMaterialCount);
		for (int iMat = 0; iMat < pFileHeader->iMaterialCount; iMat++)
		{
			uint64_t strLen = Utl::StringLength((char*)pBytes);
			char* pCurString = Utl::StringDuplicate_N((char*)pBytes, strLen);
			pOutResource.pMaterialNames[iMat] = pCurString;
			// FIXME: FREE THESE!

			currOffset += strLen + 1;
			pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);
		}

		// Clean up arrays.
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			delete[] pMeshHeaders[iMesh].pFaceIndices;
			pMeshHeaders[iMesh].pFaceIndices = nullptr;

			delete[] pMeshHeaders[iMesh].pIndexData;
			pMeshHeaders[iMesh].pIndexData = nullptr;
		}

		delete[] pMeshHeaders;
		pMeshHeaders = nullptr;

		return true;
	}

}	// namespace MeshLoader
