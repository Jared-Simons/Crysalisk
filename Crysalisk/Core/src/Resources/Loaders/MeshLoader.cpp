//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#include "MeshLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"
#include "Resources/Serializers/MaterialSerializer.h"
#include "Resources/Serializers/MeshSerializer.h"

#include <stdio.h> // For sscanf()

namespace MeshLoader
{
	// Helper functions.
	static bool LoadCModelMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource);

	static inline float UnpackF32FromUint16(const uint16_t inputData);

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
				//return LoadOBJMeshResource(szFormattedPathWithExtension, pOutResource);
				return MeshSerializer::SerializeOBJMesh(szFormattedPathWithExtension, pszMeshFileName);
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
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			pMeshHeaders[iMesh].iNumFaces = *(uint16_t*)pBytes;
			pMeshHeaders[iMesh].pFaceIndices = new uint16_t[pMeshHeaders[iMesh].iNumFaces];
			currOffset += sizeof(uint16_t) + (sizeof(uint16_t) * pMeshHeaders[iMesh].iNumFaces);
			pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);
		}

		// Mesh faces.
		MeshSerializer::CModel_MeshFaceData_t* pMeshFaces = new MeshSerializer::CModel_MeshFaceData_t[pFileHeader->iFaceDataCount];
		Platform::MemoryCopy(pMeshFaces, pBytes, pFileHeader->iFaceDataCount * pFileHeader->iFaceDataStride);
		currOffset += (pFileHeader->iFaceDataCount * pFileHeader->iFaceDataStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Unpack geometry tables.
		CAssert((pFileHeader->iPositionCount % 3) == 0);
		
		// Positions.
		uint16_t* pPackedPositions = new uint16_t[pFileHeader->iPositionCount + 3];	// +3 to avoid possible buffer overrun.
		float* pUnpackedPositions = new float[pFileHeader->iPositionCount + 3];

		Platform::MemoryCopy(pPackedPositions, pBytes, pFileHeader->iPositionCount * pFileHeader->iPositionStride);

		for (unsigned int iPos = 0; iPos < pFileHeader->iPositionCount; iPos += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedPositions[iPos + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedPositions[iPos + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedPositions[iPos + 2]);

			// Scale the positions with the mesh extents.
			float flScaledX = (flUnpackedUnitFloatX) * (pFileHeader->vecMaxExtents.x - pFileHeader->vecMinExtents.x) + pFileHeader->vecMinExtents.x;
			float flScaledY = (flUnpackedUnitFloatY) * (pFileHeader->vecMaxExtents.y - pFileHeader->vecMinExtents.y) + pFileHeader->vecMinExtents.y;
			float flScaledZ = (flUnpackedUnitFloatZ) * (pFileHeader->vecMaxExtents.z - pFileHeader->vecMinExtents.z) + pFileHeader->vecMinExtents.z;

			pUnpackedPositions[iPos + 0] = flScaledX;
			pUnpackedPositions[iPos + 1] = flScaledY;
			pUnpackedPositions[iPos + 2] = flScaledZ;
		}
		currOffset += (pFileHeader->iPositionCount * pFileHeader->iPositionStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Normals.
		uint16_t* pPackedNormals = new uint16_t[pFileHeader->iNormalCount + 3]; // +3 to avoid possible buffer overrun.
		float* pUnpackedNormals = new float[pFileHeader->iNormalCount + 3];

		Platform::MemoryCopy(pPackedNormals, pBytes, pFileHeader->iNormalCount * pFileHeader->iNormalStride);

		for (unsigned int iNormal = 0; iNormal < pFileHeader->iNormalCount; iNormal += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedNormals[iNormal + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedNormals[iNormal + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedNormals[iNormal + 2]);

			float flScaledNormalX = (flUnpackedUnitFloatX * 2.0f) - 1.0f;
			float flScaledNormalY = (flUnpackedUnitFloatY * 2.0f) - 1.0f;
			float flScaledNormalZ = (flUnpackedUnitFloatZ * 2.0f) - 1.0f;

			pUnpackedNormals[iNormal + 0] = flScaledNormalX;
			pUnpackedNormals[iNormal + 1] = flScaledNormalY;
			pUnpackedNormals[iNormal + 2] = flScaledNormalZ;
		}
		currOffset += (pFileHeader->iNormalCount * pFileHeader->iNormalStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Texcoords.
		uint16_t* pPackedTexCoords = new uint16_t[pFileHeader->iTexCoordCount + 3]; // +3 to avoid possible buffer overrun.
		float* pUnpackedTexCoords = new float[pFileHeader->iTexCoordCount + 3];

		Platform::MemoryCopy(pPackedTexCoords, pBytes, pFileHeader->iTexCoordCount * pFileHeader->iTexCoordStride);

		for (unsigned int iTexCoord = 0; iTexCoord < pFileHeader->iTexCoordCount; iTexCoord += 2)
		{
			const float flPackedTexCoordMins = -512.0f;
			const float flPackedTexCoordMaxs = 512.0f;

			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedTexCoords[iTexCoord + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedTexCoords[iTexCoord + 1]);

			flUnpackedUnitFloatX = (flUnpackedUnitFloatX * 2.0f) - 1.0f;
			flUnpackedUnitFloatY = (flUnpackedUnitFloatY * 2.0f) - 1.0f;

			float flScaledTexCoordX = (flUnpackedUnitFloatX) * (flPackedTexCoordMaxs - flPackedTexCoordMins) - flPackedTexCoordMins;
			float flScaledTexCoordY = (flUnpackedUnitFloatY) * (flPackedTexCoordMaxs - flPackedTexCoordMins) - flPackedTexCoordMins;
			pUnpackedTexCoords[iTexCoord + 0] = flScaledTexCoordX;
			pUnpackedTexCoords[iTexCoord + 1] = flScaledTexCoordY;
		}
		currOffset += (pFileHeader->iTexCoordCount * pFileHeader->iTexCoordStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Tangents.
		uint16_t* pPackedTangents = new uint16_t[pFileHeader->iTangentCount + 3]; // +3 to avoid possible buffer overrun.
		float* pUnpackedTangents = new float[pFileHeader->iTangentCount + 3];

		Platform::MemoryCopy(pPackedTangents, pBytes, pFileHeader->iTangentCount * pFileHeader->iTangentStride);

		for (unsigned int iTangent = 0; iTangent < pFileHeader->iTangentCount; iTangent += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedTangents[iTangent + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedTangents[iTangent + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedTangents[iTangent + 2]);

			float flScaledTangentX = (flUnpackedUnitFloatX * 2.0f) - 1.0f;
			float flScaledTangentY = (flUnpackedUnitFloatY * 2.0f) - 1.0f;
			float flScaledTangentZ = (flUnpackedUnitFloatZ * 2.0f) - 1.0f;

			pUnpackedTangents[iTangent + 0] = flScaledTangentX;
			pUnpackedTangents[iTangent + 1] = flScaledTangentY;
			pUnpackedTangents[iTangent + 2] = flScaledTangentZ;
		}

		// Generate faces from mesh data.
		for (unsigned int iFace = 0; iFace < pFileHeader->iFaceDataCount; iFace++)
		{
			// LEFTOFF: How to make this work across multiple submeshes?
			// If there is a single flat buffer for all the faces/verts in the model, how to split certain meshes?
			MeshSerializer::CModel_MeshFaceData_t&	pCurFace = pMeshFaces[iFace];
		}

		pOutResource.pMeshes.Resize(pFileHeader->iMeshDataCount);
		pOutResource.iVertexLayoutFlags = pFileHeader->iVertexLayoutFlags;
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			//MeshResource_t::GeometryObject_t&		pCurOutMesh = pOutResource.pMeshes[iMesh];
			//MeshSerializer::CModel_MeshHeader_t&	pCurMeshHeader = pMeshHeaders[iMesh];
		}

		FileSystem::CloseFile(&hCModelFile);

		// Clean up arrays.
		delete[] pUnpackedTangents;
		pUnpackedTangents = nullptr;
		delete[] pPackedTangents;
		pPackedTangents = nullptr;

		delete[] pUnpackedTexCoords;
		pUnpackedTexCoords = nullptr;
		delete[] pPackedTexCoords;
		pPackedTexCoords = nullptr;

		delete[] pUnpackedNormals;
		pUnpackedNormals = nullptr;
		delete[] pPackedNormals;
		pPackedNormals = nullptr;

		delete[] pUnpackedPositions;
		pUnpackedPositions = nullptr;
		delete[] pPackedPositions;
		pPackedPositions = nullptr;

		delete[] pMeshFaces;
		pMeshFaces = nullptr;

		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			delete pMeshHeaders[iMesh].pFaceIndices;
			pMeshHeaders[iMesh].pFaceIndices = nullptr;
		}

		delete[] pMeshHeaders;
		pMeshHeaders = nullptr;

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	//-------------------------------------------------------------------------
	static inline float UnpackF32FromUint16(const uint16_t inputData)
	{
		const unsigned int numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

		// Scale the data and store in a float.
		float flUnitFloat = ((float)inputData / numIntervals);

		return flUnitFloat;
	}

}	// namespace MeshLoader
