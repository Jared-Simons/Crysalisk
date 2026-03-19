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
			pMeshHeaders[iMesh].iNumFaces = *(uint32_t*)pBytes;
			pMeshHeaders[iMesh].pFaceIndices = new uint32_t[pMeshHeaders[iMesh].iNumFaces];

			currOffset += sizeof(uint32_t);
			pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

			Platform::MemoryCopy(pMeshHeaders[iMesh].pFaceIndices, pBytes, sizeof(uint32_t) * pMeshHeaders[iMesh].iNumFaces);

			currOffset += (sizeof(uint32_t) * pMeshHeaders[iMesh].iNumFaces);
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
		//uint16_t* pPackedPositions = new uint16_t[pFileHeader->iPositionCount + 3];	// +3 to avoid possible buffer overrun.
		uint16_t* pPackedPositions = new uint16_t[pFileHeader->iPositionCount];
		Vector* pAssembledPositions = new Vector[pFileHeader->iPositionCount + 3];

		Platform::MemoryCopy(pPackedPositions, pBytes, pFileHeader->iPositionCount * pFileHeader->iPositionStride);

		uint64_t iCurPosVector = 0;
		for (unsigned int iPos = 0; iPos < pFileHeader->iPositionCount; iPos += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedPositions[iPos + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedPositions[iPos + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedPositions[iPos + 2]);

			// Scale the positions with the mesh extents.
			float flScaledX = (flUnpackedUnitFloatX) * (pFileHeader->vecMaxExtents.x - pFileHeader->vecMinExtents.x) + pFileHeader->vecMinExtents.x;
			float flScaledY = (flUnpackedUnitFloatY) * (pFileHeader->vecMaxExtents.y - pFileHeader->vecMinExtents.y) + pFileHeader->vecMinExtents.y;
			float flScaledZ = (flUnpackedUnitFloatZ) * (pFileHeader->vecMaxExtents.z - pFileHeader->vecMinExtents.z) + pFileHeader->vecMinExtents.z;

			pAssembledPositions[iCurPosVector].flElements[0] = flScaledX;
			pAssembledPositions[iCurPosVector].flElements[1] = flScaledY;
			pAssembledPositions[iCurPosVector].flElements[2] = flScaledZ;
			iCurPosVector++;
		}
		currOffset += (pFileHeader->iPositionCount * pFileHeader->iPositionStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Normals.
		//uint16_t* pPackedNormals = new uint16_t[pFileHeader->iNormalCount + 3]; // +3 to avoid possible buffer overrun.
		uint16_t* pPackedNormals = new uint16_t[pFileHeader->iNormalCount];
		Vector* pAssembledNormals = new Vector[pFileHeader->iNormalCount + 3];

		Platform::MemoryCopy(pPackedNormals, pBytes, pFileHeader->iNormalCount * pFileHeader->iNormalStride);

		uint64_t iCurNormalVector = 0;
		for (unsigned int iNormal = 0; iNormal < pFileHeader->iNormalCount; iNormal += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedNormals[iNormal + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedNormals[iNormal + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedNormals[iNormal + 2]);

			float flScaledNormalX = (flUnpackedUnitFloatX * 2.0f) - 1.0f;
			float flScaledNormalY = (flUnpackedUnitFloatY * 2.0f) - 1.0f;
			float flScaledNormalZ = (flUnpackedUnitFloatZ * 2.0f) - 1.0f;

			pAssembledNormals[iCurNormalVector].flElements[0] = flScaledNormalX;
			pAssembledNormals[iCurNormalVector].flElements[1] = flScaledNormalY;
			pAssembledNormals[iCurNormalVector].flElements[2] = flScaledNormalZ;
			iCurNormalVector++;
		}
		currOffset += (pFileHeader->iNormalCount * pFileHeader->iNormalStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Texcoords.
		//uint16_t* pPackedTexCoords = new uint16_t[pFileHeader->iTexCoordCount + 3]; // +3 to avoid possible buffer overrun.
		uint16_t* pPackedTexCoords = new uint16_t[pFileHeader->iTexCoordCount];
		Vector2D* pAssembledTexCoords = new Vector2D[pFileHeader->iTexCoordCount + 3];

		Platform::MemoryCopy(pPackedTexCoords, pBytes, pFileHeader->iTexCoordCount * pFileHeader->iTexCoordStride);

		uint64_t iCurTexCoord = 0;
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
			pAssembledTexCoords[iCurTexCoord].flElements[0] = flScaledTexCoordX;
			pAssembledTexCoords[iCurTexCoord].flElements[1] = flScaledTexCoordY;
			iCurTexCoord++;
		}
		currOffset += (pFileHeader->iTexCoordCount * pFileHeader->iTexCoordStride);
		pBytes = (byte*)((uint8_t*)pDataPtr + currOffset);

		// Tangents.
		//uint16_t* pPackedTangents = new uint16_t[pFileHeader->iTangentCount + 3]; // +3 to avoid possible buffer overrun.
		uint16_t* pPackedTangents = new uint16_t[pFileHeader->iTangentCount];
		Vector* pAssembledTangents = new Vector[pFileHeader->iTangentCount + 3];

		Platform::MemoryCopy(pPackedTangents, pBytes, pFileHeader->iTangentCount * pFileHeader->iTangentStride);

		uint64_t iCurTangentVector = 0;
		for (unsigned int iTangent = 0; iTangent < pFileHeader->iTangentCount; iTangent += 3)
		{
			float flUnpackedUnitFloatX = UnpackF32FromUint16(pPackedTangents[iTangent + 0]);
			float flUnpackedUnitFloatY = UnpackF32FromUint16(pPackedTangents[iTangent + 1]);
			float flUnpackedUnitFloatZ = UnpackF32FromUint16(pPackedTangents[iTangent + 2]);

			float flScaledTangentX = (flUnpackedUnitFloatX * 2.0f) - 1.0f;
			float flScaledTangentY = (flUnpackedUnitFloatY * 2.0f) - 1.0f;
			float flScaledTangentZ = (flUnpackedUnitFloatZ * 2.0f) - 1.0f;

			pAssembledTangents[iCurTangentVector].flElements[0] = flScaledTangentX;
			pAssembledTangents[iCurTangentVector].flElements[1] = flScaledTangentY;
			pAssembledTangents[iCurTangentVector].flElements[2] = flScaledTangentZ;
			iCurTangentVector++;
		}

		pOutResource.pMeshes.Resize(pFileHeader->iMeshDataCount);
		pOutResource.pMeshes.SetCount(pFileHeader->iMeshDataCount);
		pOutResource.iVertexLayoutFlags = pFileHeader->iVertexLayoutFlags;
		for (unsigned int iMesh = 0; iMesh < pFileHeader->iMeshDataCount; iMesh++)
		{
			MeshResource_t::GeometryObject_t&		pCurOutMesh = pOutResource.pMeshes[iMesh];
			MeshSerializer::CModel_MeshHeader_t&	pCurMeshHeader = pMeshHeaders[iMesh];

			pCurOutMesh.pVertexData = new byte[uVertexStride * pCurMeshHeader.iNumFaces * 3];
			pCurOutMesh.iVertexCount = pCurMeshHeader.iNumFaces * 3;
			CAssert(pCurMeshHeader.pFaceIndices != nullptr);

			// FIXME: Figure out indexed drawing
			//pCurOutMesh.iIndexData = new uint32_t[pCurMeshHeader.iNumFaces * 3];
			//pCurOutMesh.iIndexCount = pCurMeshHeader.iNumFaces * 3;

			// LEFTOFF: There is something screwy happening here with the faces

			for (uint32_t iFace = 0; iFace < pCurMeshHeader.iNumFaces; iFace++)
			{
				//uint16_t curFaceIndex = pCurMeshHeader.pFaceIndices[iFace];
				uint16_t curFaceIndex = iFace;

				for (int iVert = 0; iVert < 3; iVert++)
				{
					Vertex3D newVert{};
					newVert.Pos = pAssembledPositions[pMeshFaces[pCurMeshHeader.pFaceIndices[curFaceIndex]].iPositionIndices[iVert]];
					newVert.Normal = pAssembledNormals[pMeshFaces[pCurMeshHeader.pFaceIndices[curFaceIndex]].iNormalIndices[iVert]];
					newVert.TexCoord = pAssembledTexCoords[pMeshFaces[pCurMeshHeader.pFaceIndices[curFaceIndex]].iTexCoordIndices[iVert]];
					newVert.Tangent = pAssembledTangents[pMeshFaces[pCurMeshHeader.pFaceIndices[curFaceIndex]].iTangentIndices[iVert]];

					// Write the vert out to the flat buffer.
					Platform::MemoryCopy(&pCurOutMesh.pVertexData[(uVertexStride * iFace * 3) + (iVert * uVertexStride)], &newVert, uVertexStride);
					// TODO: We need to precompute and serialize these!
					//pCurOutMesh.iIndexData[(iFace * 3) + iVert] = (iFace * 3) + iVert;
				}
			}
		}

		// Clean up arrays.
		delete[] pAssembledTangents;
		pAssembledTangents = nullptr;
		delete[] pPackedTangents;
		pPackedTangents = nullptr;

		delete[] pAssembledTexCoords;
		pAssembledTexCoords = nullptr;
		delete[] pPackedTexCoords;
		pPackedTexCoords = nullptr;

		delete[] pAssembledNormals;
		pAssembledNormals = nullptr;
		delete[] pPackedNormals;
		pPackedNormals = nullptr;

		delete[] pAssembledPositions;
		pAssembledPositions = nullptr;
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
