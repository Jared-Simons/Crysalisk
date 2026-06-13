//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "MaterialSerializer.h"
#include "Platform/FileSystem.h"
#include "Utils/Utils.h"

#include <stdio.h>	// for sscanf()
#include <float.h>

namespace MeshSerializer
{
	// Specific file extension for the Crysalisk model file format (CModels).
	static const char*		kCModelFileExtension = ".cmodel_c";
	// The current most up-to-date version of the CModel file format.
	static const u32	kCurrentModelVersion = 2;

	// Identifies unique vertex data
	struct UniqueVertex_t
	{
		u32 iPositionIdx{};
		u32 iNormalIdx{};
		u32 iTexCoordIdx{};
		u32 iTangentIdx{};
	};

	// Forward declarations
	static void ProcessCurrentSubmesh(Utl::CVector<CModel_MeshHeader_t>& meshHeaders, Utl::CVector<CModel_MeshFaceData_t>& meshFaces, 
									  Utl::CVector<Vector>& vertexPositions, Utl::CVector<Vector>& vertexNormals, Utl::CVector<Vector2D>& vertexTexCoords, 
									  Utl::CVector<f32>& finalVertexData, Utl::CVector<u32>& finalIndexData, u64& iUniqueVertexCount, bool bFinalSubmesh);

	static void DeduplicateGeometry(Utl::CVector<CModel_MeshFaceData_t>& pFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords, Utl::CVector<Vector>& pInTangents, Utl::CVector<UniqueVertex_t>& pOutVertices, Utl::CVector<u32>& pOutIndices);
	static void GenerateTangentData(Utl::CVector<UniqueVertex_t>& pVertexIndices, Utl::CVector<u32>& pIndices, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces);
	static bool ImportOBJMaterialLibrary(const char* pszMaterialLibraryName, MeshLoader::MeshResource_t& pMeshResource);

	//-------------------------------------------------------------------------
	// Purpose: Attempts to write an OBJ mesh out in our own mesh format.
	//-------------------------------------------------------------------------
	bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName, MeshLoader::MeshResource_t& pOutResource)
	{
		// Make sure the path is valid.
		if (!pszOBJMeshPath)
		{
			LOG_ERROR("SerializeOBJMesh() requires a path to a mesh to serialize! Mesh serialization failed.");
			return false;
		}
		
		if (!pszOutMeshFileName)
		{
			LOG_ERROR("SerializeOBJMesh() requires a path to put the output file! Mesh serialization failed!");
			return false;
		}

		FileHandle_t hOBJFile{};

		if (!FileSystem::FileExists(pszOBJMeshPath))
		{
			// Check with an extension added on, just in case.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.obj", szTempStringBuffer, pszOBJMeshPath);

			if (!FileSystem::FileExists(szTempStringBuffer))
			{
				LOG_ERROR("SerializeOBJMesh() - Source file '%s' does not exist. Mesh serialization failed.");
				return false;
			}
			else
			{
				// We found the file after adding the extension, open 'er up!
				if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_READ, false, &hOBJFile))
				{
					LOG_ERROR("Failed to open OBJ file '%s' for reading.", szTempStringBuffer);
					return false;
				}
			}
		}
		else
		{
			// Found the file.
			if (!FileSystem::OpenFile(pszOBJMeshPath, FILEMODE_READ, false, &hOBJFile))
			{
				LOG_ERROR("Failed to open OBJ file '%s' for reading.", pszOBJMeshPath);
				return false;
			}
		}

		LOG_INFO("Serializing mesh '%s'.", pszOBJMeshPath);

		// Create a new file/overwrite the existing file to store the new mesh.
		FileHandle_t hNewOutputMesh{};
		u64 iTotalBytesWritten{ 0 };
		u64 iOriginalByteSize = FileSystem::FileSize(&hOBJFile);
		char szTempStringBuffer[2048];

		// Attach our extension onto the output mesh.
		Utl::StringFormat_V("%s%s", szTempStringBuffer, pszOutMeshFileName, kCModelFileExtension);

		if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_WRITE, true, &hNewOutputMesh))
		{
			LOG_ERROR("Failed to open new file for writing mesh '%s'. Mesh serialization failed.", pszOBJMeshPath);
			return false;
		}

		CModel_FileHeader_t	outFileHeader{};
		outFileHeader.iFileVersion = kCurrentModelVersion;

		char szCurrentLine[2048]{};
		char firstChar{};
		char secondChar{};
		u64 bytesRead{ 0 };

		// Rewind the file so we can read it through again.
		FileSystem::ReturnFilePointerToHead(&hOBJFile);

		// Temp geometry tables.
		Utl::CVector<Vector>								vertexPositions{};
		Utl::CVector<Vector>								vertexNormals{};
		Utl::CVector<Vector2D>								vertexTexCoords{};
		Utl::CVector<CModel_MeshFaceData_t>					meshFaces{};

		// Geometry data.
		Utl::CVector<CModel_MeshHeader_t>	meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>	finalMeshFaces{};

		// Final geometry tables.
		Utl::CVector<Vector>	finalVertexPositions{};
		Utl::CVector<Vector>	finalVertexNormals{};
		Utl::CVector<Vector2D>	finalVertexTexCoords{};
		Utl::CVector<Vector>	finalVertexTangents{};

		Utl::CVector<f32>		finalVertexData{};
		Utl::CVector<u32>	finalIndexData{};
		u64				iUniqueVertCounter{ 0 };

		// Material stuff.
		// TEMP DEBUG
		bool debugNoGenerateMaterials = false;
		char* pszMTLFilePath{ nullptr };
		// Contains all of the material names encountered (does not contain repeats!)
		Utl::CVector<char*>		materialNames{};
		Utl::CVector<u32>	materialNameIndices{};
		u32				iMaterialRunningTotal{ 0 };
		char*					pszMaterialName{ nullptr };
		bool					bMatFound = false;

		Utl::CVector<char*> outputStringTable{};
		u64			iStringTableRunningBytes{ 0 };

		Platform::MemorySet(szCurrentLine, 0, sizeof(char) * 2048);
		bytesRead = 0;

		meshHeaders.PushBack(CModel_MeshHeader_t{});	// Setup first geo group.
		
		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile))
		{
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];

			switch (firstChar)
			{
			case '#':	// Skip comments.
			case 0:		// And blank lines
				break;

			case 'g':	// New geometry object.
			{
				{
					if (meshHeaders.Count() > 0 && meshFaces.Count() > 0)
					{
						ProcessCurrentSubmesh(meshHeaders, meshFaces, vertexPositions, vertexNormals, vertexTexCoords, finalVertexData, finalIndexData, iUniqueVertCounter, false);
					}
					char szMeshName[512];
					Utl::StringFormat_V("%s", szMeshName, &szCurrentLine[2]);

					// Add it to the string table.
					outputStringTable.PushBack("");
					outputStringTable.Back() = Utl::StringDuplicate(szMeshName);
					meshHeaders.Back().iMeshNameIndex = (u32)outputStringTable.Count() - 1;
				}
				continue;
			}

			case 'f':	// Polygon 'face'.
			{
				u32	positionIndices[3];
				u32	normalIndices[3];
				u32	texCoordIndices[3];

				i32 elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &positionIndices[0], &texCoordIndices[0], &normalIndices[0], &positionIndices[1], &texCoordIndices[1], &normalIndices[1], &positionIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				// Write the elements out to the mesh header.
				CModel_MeshFaceData_t newFace{};
				for (i32 iVert = 0; iVert < 3; iVert++)
				{
					// NOTE: OBJ files are 1-indexed so we need to bump down by one to store as a zero index.
					newFace.iPositionIndices[iVert] = positionIndices[iVert] - 1;
					newFace.iNormalIndices[iVert] = normalIndices[iVert] - 1;
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert] - 1;
				}
				meshFaces.PushBack(newFace);
				break;
			}

			case 'u':
				// New Material.

				// If we are currently processing an object, save it off and start a new one.
				// Each object should have one unique material.
				if (meshHeaders.Count() > 0 && meshFaces.Count() > 0)
				{
					ProcessCurrentSubmesh(meshHeaders, meshFaces, vertexPositions, vertexNormals, vertexTexCoords, finalVertexData, finalIndexData, iUniqueVertCounter, false);

					// Use the previous mesh's name as our name.
					u64 iCurIdx = outputStringTable.PushBack("");
					outputStringTable.Back() = Utl::StringDuplicate(outputStringTable[meshHeaders[meshHeaders.Count() - 2].iMeshNameIndex]);
					meshHeaders.Back().iMeshNameIndex = (u32)outputStringTable.Count() - 1;
				}

				// Optimize, maybe this could be made faster with a hashmap??
				// This could become taxing on VERY large models.

				// Get just the material name.
				pszMaterialName = &szCurrentLine[7];
				bMatFound = false;

				// Check to see if we've seen this material yet.
				for (i32 iMat = 0; iMat < materialNames.Count(); iMat++)
				{
					if (Utl::StringCompare(pszMaterialName, materialNames[iMat]))
					{
						// This material already exists in the table!
						bMatFound = true;
						meshHeaders.Back().iMaterialNameIndex = iMat;
						break;
					}
				}

				// This material still needs to be added.
				if (!bMatFound)
				{
					outputStringTable.PushBack(Utl::StringDuplicate(pszMaterialName));
					meshHeaders.Back().iMaterialNameIndex = iMaterialRunningTotal;
					iMaterialRunningTotal++;
					materialNameIndices.PushBack((u32)(u32)outputStringTable.Count() - 1);
					break;
				}
				else
				{
					// Just break out.
					break;
				}

			case 's':	// Smoothing groups, not using these.
				continue;

			case 'v':
			{
				// Need to check the second char.
				switch (secondChar)
				{
					case ' ':	// Vertex position.
					{
						Vector newPosition{};
						i32 numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newPosition.x, &newPosition.y, &newPosition.z);
						CAssert(numElements == 3);

						// Add it to the table.
						vertexPositions.PushBack(newPosition);
						break;
					}

					case 'n':	// Vertex normals.
					{
						Vector newNormal{};
						i32 numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);
						CAssert(numElements == 3);

						// When we serialize these, we're gonna quantize with -1.0 and 1.0 being our bounds.
						CAssert(newNormal.x <= 1.0f && newNormal.x >= -1.0f);
						CAssert(newNormal.y <= 1.0f && newNormal.y >= -1.0f);
						CAssert(newNormal.z <= 1.0f && newNormal.z >= -1.0f);

						// Add it to the table.
						vertexNormals.PushBack(newNormal);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						i32 numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);
						CAssert(numElements == 2);

						// When we serialize these, we're gonna quantize with -512.0 and 512.0 being our bounds. (We're being VERY generous with repeated textures).
						CAssert(newTexCoord.x <= 512.0f && newTexCoord.x >= -512.0f);
						CAssert(newTexCoord.y <= 512.0f && newTexCoord.y >= -512.0f);

						// Add it to the table.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}
				}
				break;
			}

			case 'm':
				if (debugNoGenerateMaterials)
					break;
				// Append the appropriate extension to the path.
				// NOTE: This assumes that the mtllib file has the same name as the OBJ.
				char* pszMTLFilePath = Utl::StringDuplicate(pszOBJMeshPath);
				u64 iStringLength = Utl::StringLength(pszMTLFilePath);
				pszMTLFilePath[iStringLength - 3] = 'm';
				pszMTLFilePath[iStringLength - 2] = 't';
				pszMTLFilePath[iStringLength - 1] = 'l';
				pszMTLFilePath[iStringLength - 0] = '\0';
				if (!ImportOBJMaterialLibrary(pszMTLFilePath, pOutResource))
				{
					LOG_ERROR("Failed to import '%s' material library! Will use default material.", pszMTLFilePath);
				}
				break;
			}
		}

		// Serialize the final mesh (this won't be handled in the switch statement).
		ProcessCurrentSubmesh(meshHeaders, meshFaces, vertexPositions, vertexNormals, vertexTexCoords, finalVertexData, finalIndexData, iUniqueVertCounter, true);

		// Calculate the complete model extents.
		outFileHeader.vecMinExtents = Vector(kFLT_MAX, kFLT_MAX, kFLT_MAX);
		outFileHeader.vecMaxExtents = Vector(kFLT_MIN, kFLT_MIN, kFLT_MIN);
		for (u32 iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			outFileHeader.vecMinExtents.x = FloatMin(outFileHeader.vecMinExtents.x, meshHeaders[iMesh].vecMinExtents.x);
			outFileHeader.vecMinExtents.y = FloatMin(outFileHeader.vecMinExtents.y, meshHeaders[iMesh].vecMinExtents.y);
			outFileHeader.vecMinExtents.z = FloatMin(outFileHeader.vecMinExtents.z, meshHeaders[iMesh].vecMinExtents.z);

			outFileHeader.vecMaxExtents.x = FloatMax(outFileHeader.vecMaxExtents.x, meshHeaders[iMesh].vecMaxExtents.x);
			outFileHeader.vecMaxExtents.y = FloatMax(outFileHeader.vecMaxExtents.y, meshHeaders[iMesh].vecMaxExtents.y);
			outFileHeader.vecMaxExtents.z = FloatMax(outFileHeader.vecMaxExtents.z, meshHeaders[iMesh].vecMaxExtents.z);
		}

		LOG_INFO("Compressing mesh data.");

		// Pack vectors into uints.
		Utl::CVector<u16>	packedPositions{};
		Utl::CVector<u16>	packedNormals{};
		Utl::CVector<u16>	packedTexCoords{};
		Utl::CVector<u16>	packedTangents{};
		Utl::CVector<u16>	packedVertexData{};

		LOG_DEBUG("Packing vertex data.");
		u8 floatIdx = 0;
		for (u32 iFloat = 0; iFloat < finalVertexData.Count(); iFloat++)
		{
			f32* pCurFloat = &finalVertexData[iFloat];

			// Packing behavior differs depending on the vertex attribute.
			// Currently, this block of code is assuming the following layout: 
			// 3 floats for position, 
			// 3 floats for normal, 
			// 2 floats for texcoord, 
			// 3 floats for tangent.
			// NOTE: This code will need to change when we want to support other mesh attributes, such as anim data or optional tinting.
			f32 flNormalized = 0.0f;
			u16 packedPos = 0;
			switch (floatIdx)
			{
			case 0:		// Position case
					flNormalized = (*pCurFloat - outFileHeader.vecMinExtents.x) / (outFileHeader.vecMaxExtents.x - outFileHeader.vecMinExtents.x);
					packedPos = PackF32ToUint16(flNormalized);
					packedVertexData.PushBack(packedPos);
					break;
			case 1:
					flNormalized = (*pCurFloat - outFileHeader.vecMinExtents.y) / (outFileHeader.vecMaxExtents.y - outFileHeader.vecMinExtents.y);
					packedPos = PackF32ToUint16(flNormalized);
					packedVertexData.PushBack(packedPos);
					break;
			case 2:
					flNormalized = (*pCurFloat - outFileHeader.vecMinExtents.z) / (outFileHeader.vecMaxExtents.z - outFileHeader.vecMinExtents.z);
					packedPos = PackF32ToUint16(flNormalized);
					packedVertexData.PushBack(packedPos);
					break;
				break;

			case 3:		// Normal case
			case 4:
			case 5:
				{
					flNormalized = (*pCurFloat + 1.0f) / 2.0f;
					u16 packedNormal = PackF32ToUint16(flNormalized);
					packedVertexData.PushBack(packedNormal);
				}
				break;

			case 6:		// Texcoord case
			case 7:
				{
					const f32 flPackedTexCoordMins = -64.0f;
					const f32 flPackedTexCoordMaxs = 64.0f;

					flNormalized = (*pCurFloat - flPackedTexCoordMins) / (flPackedTexCoordMaxs - flPackedTexCoordMins);
					u16 packedTexcoord = PackF32ToUint16(flNormalized);
					packedVertexData.PushBack(packedTexcoord);
				}
				break;

			case 8:		// Tangent case
			case 9:
			case 10:
			{
				flNormalized = (*pCurFloat + 1.0f) / 2.0f;
				u16 packedTangent = PackF32ToUint16(flNormalized);
				packedVertexData.PushBack(packedTangent);
			}
			break;

			default:
				CAssert(0);
				break;
			}

			floatIdx = (floatIdx + 1) % 11;
		}

		// Before writing out the data, map all the string indices to byte offsets
		// so that the strings can be loaded from a single flat buffer.
		u32 stringCount = (u32)outputStringTable.Count();
		Utl::CVector<u32>	stringIndexToBytes(stringCount);
		stringIndexToBytes.SetCount(stringCount);

		for (u32 iStr = 0; iStr < stringCount; iStr++)
		{
			stringIndexToBytes[iStr] = (u32)iStringTableRunningBytes;

			u64 strLength = Utl::StringLength(outputStringTable[iStr]);
			iStringTableRunningBytes += (strLength + 1);	// Including the null terminator.
		}

		// Convert the mesh name indices to byte offsets into the final string buffer.
		for (u32 iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			u32 meshNameIndex = meshHeaders[iMesh].iMeshNameIndex;
			meshHeaders[iMesh].iMeshNameIndex = stringIndexToBytes[meshNameIndex];
		}

		// Setup offsets/counts.
		outFileHeader.iVertexStride = sizeof(Vertex3D);
		outFileHeader.iVertexCount = (u32)packedVertexData.Count();
		outFileHeader.iIndexCount = (u32)finalIndexData.Count();
		outFileHeader.iIndexStride = (u32)finalIndexData.Stride();
		outFileHeader.iMaterialCount = iMaterialRunningTotal;

		outFileHeader.iMeshDataCount = (u32)meshHeaders.Count();
		outFileHeader.iMeshDataStride = (u8)meshHeaders.Stride();

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		// Write out the file header.
		FileSystem::WriteBytes(&outFileHeader, sizeof(CModel_FileHeader_t), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(CModel_FileHeader_t);

		// Write out a marker for the submesh headers.
		FileSystem::WriteBytes(&kModelResourceSubmeshHeader, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		for (i32 iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			FileSystem::WriteBytes(&meshHeaders[iMesh], sizeof(CModel_MeshHeader_t), &hNewOutputMesh);
			iTotalBytesWritten += sizeof(CModel_MeshHeader_t);
		}

		// Write out a marker for the vertex data.
		FileSystem::WriteBytes(&kModelResourceVertexDataHeader, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		FileSystem::WriteBytes(packedVertexData.Data(), packedVertexData.Stride() * packedVertexData.Count(), &hNewOutputMesh);
		iTotalBytesWritten += packedVertexData.Stride() * packedVertexData.Count();

		// Write out a marker for the index data.
		FileSystem::WriteBytes(&kModelResourceIndexDataHeader, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		FileSystem::WriteBytes(finalIndexData.Data(), finalIndexData.Stride() * finalIndexData.Count(), &hNewOutputMesh);
		iTotalBytesWritten += finalIndexData.Stride() * finalIndexData.Count();

		// Write out a marker for the material data.
		FileSystem::WriteBytes(&kModelResourceMaterialDataHeader, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		// Write out an array that contains the offsets of ONLY the material names
		// This allows us to quickly get a list of materials this model uses.
		for (u32 iMat = 0; iMat < iMaterialRunningTotal; iMat++)
		{
			FileSystem::WriteBytes(&stringIndexToBytes[materialNameIndices[iMat]], sizeof(u32), &hNewOutputMesh);
			iTotalBytesWritten += sizeof(u32);
		}

		// Write out a marker for the string data.
		FileSystem::WriteBytes(&kModelResourceStringDataHeader, sizeof(u32), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(u32);

		for (i32 idx = 0; idx < outputStringTable.Count(); idx++)
		{
			u64 len = Utl::StringLength(outputStringTable[idx]);
			FileSystem::WriteBytes(outputStringTable[idx], len + 1, &hNewOutputMesh);
			iTotalBytesWritten += (len + 1);
		}

		// Close the new file.
		FileSystem::CloseFile(&hNewOutputMesh);

		// Close the original source file.
		FileSystem::CloseFile(&hOBJFile);

		// Clean up strings.
		for (i32 i = 0; i < outputStringTable.Count(); i++)
		{
			Utl::StringFree(outputStringTable[i]);
			outputStringTable[i] = nullptr;
		}
		outputStringTable.Clear();

		f32 flPercentReduced = ((f32)iTotalBytesWritten / iOriginalByteSize);
		flPercentReduced *= 100.0f;
		LOG_INFO("Mesh '%s' compressed from %d bytes to %d bytes ->(%.2f%%)", pszOBJMeshPath, iOriginalByteSize, iTotalBytesWritten, flPercentReduced);

		return MeshLoader::LoadMeshResource((char*)pszOutMeshFileName, pOutResource);
	}

	static void ProcessCurrentSubmesh(Utl::CVector<CModel_MeshHeader_t>& meshHeaders, Utl::CVector<CModel_MeshFaceData_t>& meshFaces,
		Utl::CVector<Vector>& vertexPositions, Utl::CVector<Vector>& vertexNormals, Utl::CVector<Vector2D>& vertexTexCoords,
		Utl::CVector<f32>& finalVertexData, Utl::CVector<u32>& finalIndexData, u64& iUniqueVertexCount, bool bFinalSubmesh)
	{
		CModel_MeshHeader_t& pCurHeader = meshHeaders.Back();

		Utl::CVector<u32>			uniqueIndices{};
		Utl::CVector<UniqueVertex_t>	finalVertexIndices{};
		Utl::CVector<Vector>			generatedTangents{};
		DeduplicateGeometry(meshFaces, vertexPositions, vertexNormals, vertexTexCoords, generatedTangents, finalVertexIndices, uniqueIndices);
		GenerateTangentData(finalVertexIndices, uniqueIndices, vertexPositions, vertexNormals, vertexTexCoords, generatedTangents, meshFaces);

		pCurHeader.iVertexCount = (u32)finalVertexIndices.Count();
		pCurHeader.iVertexOffset = iUniqueVertexCount;
		pCurHeader.iIndexCount = (u32)uniqueIndices.Count();
		pCurHeader.iIndexOffset = sizeof(u32) * finalIndexData.Count();	// This is the byte offset into the index data.

		u64 indexAlignmentOffset = 0;
		if (pCurHeader.iIndexOffset % 4 != 0)
		{
			indexAlignmentOffset = 4 - (pCurHeader.iIndexOffset % 4);
			pCurHeader.iIndexOffset += indexAlignmentOffset;
		}

		for (i32 idx = 0; idx < indexAlignmentOffset; idx++)
		{
			// Fill in some blank data to the index buffer to make sure we're aligned.
			finalIndexData.PushBack((u32)0);
		}

		u32 indexBufferBase = (u32)iUniqueVertexCount;
		iUniqueVertexCount += finalVertexIndices.Count();

		// Calculate the extents of the submesh.
		pCurHeader.vecMinExtents = Vector(kFLT_MAX, kFLT_MAX, kFLT_MAX);
		pCurHeader.vecMaxExtents = -pCurHeader.vecMinExtents;
		for (u32 iVert = 0; iVert < finalVertexIndices.Count(); iVert++)
		{
			Vector3D& pCurPos = vertexPositions[finalVertexIndices[iVert].iPositionIdx];

			pCurHeader.vecMinExtents.x = FloatMin(pCurHeader.vecMinExtents.x, pCurPos.x);
			pCurHeader.vecMinExtents.y = FloatMin(pCurHeader.vecMinExtents.y, pCurPos.y);
			pCurHeader.vecMinExtents.z = FloatMin(pCurHeader.vecMinExtents.z, pCurPos.z);

			pCurHeader.vecMaxExtents.x = FloatMax(pCurHeader.vecMaxExtents.x, pCurPos.x);
			pCurHeader.vecMaxExtents.y = FloatMax(pCurHeader.vecMaxExtents.y, pCurPos.y);
			pCurHeader.vecMaxExtents.z = FloatMax(pCurHeader.vecMaxExtents.z, pCurPos.z);
		}

		// Write out the unique data to the output tables.
		for (u32 iVert = 0; iVert < finalVertexIndices.Count(); iVert++)
		{
			finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].x);
			finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].y);
			finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].z);

			finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].x);
			finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].y);
			finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].z);

			finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].x);
			finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].y);

			finalVertexData.PushBack(generatedTangents[finalVertexIndices[iVert].iTangentIdx].x);
			finalVertexData.PushBack(generatedTangents[finalVertexIndices[iVert].iTangentIdx].y);
			finalVertexData.PushBack(generatedTangents[finalVertexIndices[iVert].iTangentIdx].z);
		}

		for (u32 idx = 0; idx < uniqueIndices.Count(); idx++)
		{
			finalIndexData.PushBack(uniqueIndices[idx] + indexBufferBase);
		}
		meshFaces.Clear();

		if (!bFinalSubmesh)
		{
			meshHeaders.PushBack(CModel_MeshHeader_t{});
		}
	}

	static void DeduplicateGeometry(Utl::CVector<CModel_MeshFaceData_t>& pFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords, Utl::CVector<Vector>& pInTangents, Utl::CVector<UniqueVertex_t>& pOutVertices, Utl::CVector<u32>& pOutIndices)
	{
		Utl::CVector<UniqueVertex_t> uniqueVertices{};
		u32 iPosIdxBase = pFaces[0].iPositionIndices[0];
		u32 iNormIdxBase = pFaces[0].iNormalIndices[0];
		u32 iTexCoordIdxBase = pFaces[0].iTexCoordIndices[0];

		for (i32 iFace = 0; iFace < pFaces.Count(); iFace++)
		{
			for (i32 iVert = 0; iVert < 3; iVert++)
			{
				bool bUnique = true;

				UniqueVertex_t currentVert{};
				currentVert.iPositionIdx = pFaces[iFace].iPositionIndices[iVert];
				currentVert.iNormalIdx = pFaces[iFace].iNormalIndices[iVert];
				currentVert.iTexCoordIdx = pFaces[iFace].iTexCoordIndices[iVert];
				currentVert.iTangentIdx = pFaces[iFace].iTangentIndices[iVert];

				{
					if (currentVert.iPositionIdx < iPosIdxBase)
						iPosIdxBase = currentVert.iPositionIdx;
					if (currentVert.iNormalIdx < iNormIdxBase)
						iNormIdxBase = currentVert.iNormalIdx;
					if (currentVert.iTexCoordIdx < iTexCoordIdxBase)
						iTexCoordIdxBase = currentVert.iTexCoordIdx;
				}

				i32 iUniqueVert = 0;
				for (iUniqueVert = 0; iUniqueVert < uniqueVertices.Count(); iUniqueVert++)
				{
					if (uniqueVertices[iUniqueVert].iPositionIdx == currentVert.iPositionIdx &&
						uniqueVertices[iUniqueVert].iNormalIdx == currentVert.iNormalIdx &&
						uniqueVertices[iUniqueVert].iTexCoordIdx == currentVert.iTexCoordIdx &&
						uniqueVertices[iUniqueVert].iTangentIdx == currentVert.iTangentIdx)
					{
						// Duplicate vert found!
						bUnique = false;
						break;
					}
				}

				if (bUnique)
				{
					// Add it to the table.
					uniqueVertices.PushBack(currentVert);
					pOutIndices.PushBack(iUniqueVert);
				}
				else
				{
					// Not unique, need to reassign face indices.
					pOutIndices.PushBack(iUniqueVert);
				}
			}
		}

		// At this point, the index buffer is properly setup, now just write out the unique vertices.
		for (i32 iVert = 0; iVert < uniqueVertices.Count(); iVert++)
		{
			pOutVertices.PushBack(uniqueVertices[iVert]);
		}
	}

	static void GenerateTangentData(Utl::CVector<UniqueVertex_t>& pVertexIndices, Utl::CVector<u32>& pIndices, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces)
	{
		pOutTangents.Resize(pVertexIndices.Count());
		pOutTangents.SetCount(pVertexIndices.Count());
		for (i32 idx = 0; idx < pIndices.Count(); idx += 3)
		{
			u32& pIdx0 = pIndices[idx + 0];
			u32& pIdx1 = pIndices[idx + 1];
			u32& pIdx2 = pIndices[idx + 2];

			Vector& pPos0 = pPositions[pVertexIndices[pIdx0].iPositionIdx];
			Vector& pPos1 = pPositions[pVertexIndices[pIdx1].iPositionIdx];
			Vector& pPos2 = pPositions[pVertexIndices[pIdx2].iPositionIdx];

			Vector2D& pUV0 = pTexCoords[pVertexIndices[pIdx0].iTexCoordIdx];
			Vector2D& pUV1 = pTexCoords[pVertexIndices[pIdx1].iTexCoordIdx];
			Vector2D& pUV2 = pTexCoords[pVertexIndices[pIdx2].iTexCoordIdx];

			Vector edge1 = pPos1 - pPos0;
			Vector edge2 = pPos2 - pPos0;

			Vector2D deltaUV1 = pUV1 - pUV0;
			Vector2D deltaUV2 = pUV2 - pUV0;

			f32 flDividend = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
			f32 f = 1.0f / flDividend;

			Vector vecTangent{};
			vecTangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			vecTangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			vecTangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			pOutTangents[pIdx0] += vecTangent;
			pOutTangents[pIdx1] += vecTangent;
			pOutTangents[pIdx2] += vecTangent;

			// Also need to assign the index to the vertex.
			pVertexIndices[pIdx0].iTangentIdx = pIdx0;
			pVertexIndices[pIdx1].iTangentIdx = pIdx1;
			pVertexIndices[pIdx2].iTangentIdx = pIdx2;
		}

		// Normalize all the new tangent data
		for (i32 idx = 0; idx < pOutTangents.Count(); idx++)
		{
			pOutTangents[idx].NormalizeInPlace();
		}
	}

	static bool ImportOBJMaterialLibrary(const char* pszMaterialLibraryName, MeshLoader::MeshResource_t& pMeshResource)
	{
		CASSERTMSG(FileSystem::FileExists(pszMaterialLibraryName), "Missing OBJ Material File?");
		FileHandle_t hMtlFile{};

		if (!FileSystem::OpenFile(pszMaterialLibraryName, eFileMode::FILEMODE_READ, false, &hMtlFile))
		{
			LOG_ERROR("Failed to open OBJ MTL LIB file: '%s'.", pszMaterialLibraryName);
			return false;
		}
		
		// Read through the file.
		char	szCurrentLine[2048];
		u64	iBytesRead{ 0 };
		u64	iCurrentLineLength{ 0 };
		char	szKey[512];
		char	szValue[512];

		Utl::CVector<MaterialLoader::MaterialResource_t> processedMaterialEntries{};
		processedMaterialEntries.Resize(512); // FIXME: Don't hardcode this!! We need to fix how string memory is managed instead.

		while (FileSystem::ReadLine(szCurrentLine, 2048, &iBytesRead, &hMtlFile))
		{
			iCurrentLineLength = Utl::StringLength(szCurrentLine);
			// Skip blank lines and comments.
			if (iCurrentLineLength <= 0 || szCurrentLine[0] == '#')
				continue;

			u64 iSpaceIndex = Utl::StringIndexOf(szCurrentLine, ' ');
			CAssert(iSpaceIndex != kInvalidID_U32);

			// Split into key/value pairs.
			Platform::MemoryCopy(szKey, szCurrentLine, iSpaceIndex);
			szKey[iSpaceIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[iSpaceIndex + 1], iBytesRead - iSpaceIndex);
			szValue[iBytesRead - 1] = '\0';

			char* pszTrimmedKey = Utl::StringTrim(szKey);

			if (Utl::StringCompare(pszTrimmedKey, "newmtl"))	// New material declaration.
			{
				processedMaterialEntries.PushBack(MaterialLoader::MaterialResource_t{});
				processedMaterialEntries.Back().pszMaterialName = Utl::StringDuplicate(szValue);
				processedMaterialEntries.Back().pszShaderName = Utl::StringDuplicate("Core.Phong");		// Default hardcoded shader.
				processedMaterialEntries.Back().textureMapConfigs.Resize(3);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_Kd"))	// Diffuse map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_Ks"))	// Specular map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::SPECULAR;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_bump"))	// Normal map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::NORMAL;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "Ns"))	// Specular exponent.
			{
				processedMaterialEntries.Back().pszCustomKeys.PushBack(Utl::StringDuplicate("specular_shininess"));
				processedMaterialEntries.Back().pszCustomValues.PushBack(Utl::StringDuplicate(szValue));
			}

			Utl::StringFree(pszTrimmedKey);
			pszTrimmedKey = nullptr;
		}

		FileSystem::CloseFile(&hMtlFile);

		// Write out all of these materials.
		for (i32 iMat = 0; iMat < processedMaterialEntries.Count(); iMat++)
		{
			LOG_INFO("Saving material '%s'.", processedMaterialEntries[iMat].pszMaterialName);
			if (!MaterialSerializer::SerializeMaterialResources(processedMaterialEntries[iMat].pszMaterialName, processedMaterialEntries[iMat]))
			{
				LOG_ERROR("Failed to serialize material resource '%s'...", processedMaterialEntries[iMat].pszMaterialName);
			}
		}

		return true;
	}

}	// namespace MeshSerializer
