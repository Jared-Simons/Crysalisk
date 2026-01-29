//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "Platform/FileSystem.h"

#include <stdio.h>	// for sscanf()
#include <float.h>

namespace MeshSerializer
{
	// Specific file extension for the Crysalisk model file format (CModels).
	static const char*		kCModelFileExtension = ".cmodel_c";
	// The current most up-to-date version of the CModel file format.
	static const uint32_t	kCurrentModelVersion = 2;

	// Forward declarations
	static void GenerateTangentData(Vector* pPositions, Vector* pNormals, Vector2D* pTexCoords, Utl::CVector<Vector>* pOutTangents, uint64_t iFaceCount, CModel_MeshFaceData_t* pFaces);
	static void DeduplicateGeometry(Vector* pPositions, Vector* pNormals, Vector2D* pTexCoords, CModel_MeshFaceData_t* pFaces);

	//-------------------------------------------------------------------------
	// Purpose: Takes an existing mesh resource imported from another file type, 
	// and writes it out to disk in our own binary format.
	// 
	// Input:	pszModelName - The name of the model.
	//			pInResource - The resource data to be serialized.
	// Output:	bool - Returns true on successful serialization, otherwise false.
	//-------------------------------------------------------------------------
	bool SerializeMeshResource(const char* pszModelName, MeshLoader::MeshResource_t& pInResource)
	{
		// Create and setup the file.

		// Chop off the file extension.
		size_t nameLength = Utl::StringLength(pszModelName);
		size_t extensionLength = Utl::StringLength(kCModelFileExtension);

		char* pTempNameBuffer = new char[nameLength + extensionLength + 1];
		Utl::StringFormat_V("%s%s", pTempNameBuffer, pszModelName, kCModelFileExtension);

		FileHandle_t pCModelFileHandle{};
		if (!FileSystem::OpenFile(pTempNameBuffer, eFileMode::FILEMODE_WRITE, true, &pCModelFileHandle))
		{
			LOG_ERROR("Failed to write CModel for model '%s'.", pszModelName);
			return false;
		}

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &pCModelFileHandle);

		// Get the total number of vertices in this model.
		uint32_t totalVerts{ 0 };

		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx].pGroups.Count(); gpIdx++)
			{
				totalVerts += (uint32_t)pInResource.pObjects[objIdx].pGroups[gpIdx].Vertices.Count();
			}
		}

		// Get the total number of submeshes in this model.
		uint32_t totalSubmeshes{ 0 };

		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			totalSubmeshes += (uint32_t)pInResource.pObjects[objIdx].pGroups.Count();
		}

		// Write out the header.
		CModel_FileHeader_t fileHeader{};
		fileHeader.iFileVersion = kCurrentModelVersion;

		// Submesh data info.
		fileHeader.iSubmeshDataOffset = sizeof(CModel_FileHeader_t);
		fileHeader.iSubmeshDataStride = sizeof(MeshLoader::MeshResource_t::GeometryObject_t);
		fileHeader.iSubmeshDataSize = totalSubmeshes;

		FileSystem::WriteBytes(&fileHeader, sizeof(CModel_FileHeader_t), &pCModelFileHandle);

		// Write out all of the submesh data.
		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx].pGroups.Count(); gpIdx++)
			{
				CModel_SubmeshHeader_t subMesh{};

				subMesh.iVertexCount = pInResource.pObjects[objIdx].pGroups[gpIdx].Vertices.Count();
				subMesh.iVertexStride = pInResource.pObjects[objIdx].pGroups[gpIdx].Vertices.Stride();

				subMesh.iIndexCount = pInResource.pObjects[objIdx].pGroups[gpIdx].Indices.Count();
				subMesh.iIndexStride = pInResource.pObjects[objIdx].pGroups[gpIdx].Indices.Stride();

				// Material data.
				if (pInResource.pObjects[objIdx].pGroups[gpIdx].pszDebugName)
					subMesh.iSubmeshNameLength = Utl::StringLength(pInResource.pObjects[objIdx].pGroups[gpIdx].pszDebugName);
				else
				{
					// Add in a string for debugging purposes.
					static const char* pszAssetDirectory = "Assets/Models";
					static size_t iDirectoryLength = Utl::StringLength(pszAssetDirectory);
					char szNewDebugString[512]{};
					Utl::StringFormat_V("%s_submesh_%i", szNewDebugString, &pszModelName[iDirectoryLength], gpIdx);

					subMesh.iSubmeshNameLength = Utl::StringLength(szNewDebugString);
				}

				if (pInResource.pObjects[objIdx].pGroups[gpIdx].pszMaterialName)
					subMesh.iMaterialNameLength = Utl::StringLength(pInResource.pObjects[objIdx].pGroups[gpIdx].pszMaterialName);

				// Write the header, then the data.
				FileSystem::WriteBytes(&subMesh, sizeof(CModel_SubmeshHeader_t), &pCModelFileHandle);
				FileSystem::WriteBytes(pInResource.pObjects[objIdx].pGroups[gpIdx].pszDebugName, subMesh.iSubmeshNameLength, &pCModelFileHandle);
				FileSystem::WriteBytes(pInResource.pObjects[objIdx].pGroups[gpIdx].pszMaterialName, subMesh.iMaterialNameLength, &pCModelFileHandle);

				FileSystem::WriteBytes(pInResource.pObjects[objIdx].pGroups[gpIdx].Vertices.Data(), subMesh.iVertexCount * subMesh.iVertexStride, &pCModelFileHandle);
				FileSystem::WriteBytes(pInResource.pObjects[objIdx].pGroups[gpIdx].Indices.Data(), subMesh.iIndexCount * subMesh.iIndexStride, &pCModelFileHandle);
			}
		}

		FileSystem::CloseFile(&pCModelFileHandle);

		delete[] pTempNameBuffer;

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	// 
	// Mesh serialization algorithm:
	// - Verify that source file exists						[X]
	// - Determine the mesh's vertex layout					[X]
	// - Process all elements into their own tables			[ ]
	// - Write out the file header							[X]
	//		(Note that this has to happen after processing 
	//		elements so that we can get their offsets 
	//		into the byte-stream).
	// - Write out all tables								[ ]
	//-------------------------------------------------------------------------
	bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName)
	{
		// Make sure the path is valid.
		// We can technically proceed without having an output file name, 
		// we'll just make it the same as the input file name with a new extension.
		if (!pszOBJMeshPath)
		{
			LOG_ERROR("SerializeOBJMesh() requires a path to a mesh to serialize! Mesh serialization failed.");
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
		if (pszOutMeshFileName == nullptr)
		{
			// Just steal the input file path and put our extension on it.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.%s", szTempStringBuffer, pszOBJMeshPath, kCModelFileExtension);	// TODO: We might need to strip the original OBJ extension off the input path.
			CAssert(0);	// We need to test this.
		}
		else
		{
			// Attach our extension onto the output mesh.
			char szTempStringBuffer[2048];
			Utl::StringFormat_V("%s%s", szTempStringBuffer, pszOutMeshFileName, kCModelFileExtension);
			if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_WRITE, true, &hNewOutputMesh))
			{
				LOG_ERROR("Failed to open new file for writing mesh '%s'. Mesh serialization failed.", pszOBJMeshPath);
				return false;
			}
		}

		CModel_FileHeader_t	outFileHeader{};
		outFileHeader.iFileVersion = kCurrentModelVersion;

		// First determine the vertex format for this model.
		// This makes the assumption that there is a singular vertex layout that is valid for the entire file.
		bool bModelContainsVertexPositions{ false };
		bool bModelContainsVertexNormals{ false };
		bool bModelContainsVertexTexCoords{ false };
		bool bModelContainsVertexTangents{ false };

		char szCurrentLine[2048]{};
		char firstChar{};
		char secondChar{};
		size_t bytesRead{ 0 };
		
		LOG_INFO("Determing vertex layout");

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile))
		{
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];
			if (firstChar == 'v')
			{
				// Need to take a look at the second char to confirm.
				if (secondChar == ' ')
				{
					// Vertex position.
					bModelContainsVertexPositions = true;
				}
				else if (secondChar == 'n')
				{
					// Vertex normal.
					bModelContainsVertexNormals = true;
				}
				else if (secondChar == 't')
				{
					// Vertex texcoords.
					bModelContainsVertexTexCoords = true;
				}
			}
		}

		// Spew some vertex layout information:
		LOG_DEBUG("Mesh contains position data: \t%s", bModelContainsVertexPositions ? "true" : "false");
		LOG_DEBUG("Mesh contains normal data: \t%s", bModelContainsVertexNormals ? "true" : "false");
		LOG_DEBUG("Mesh contains texcoord data: \t%s", bModelContainsVertexTexCoords ? "true" : "false");
		LOG_DEBUG("Mesh contains tangent data: \t%s", bModelContainsVertexTangents ? "true" : "false");

		// Build a bitfield representing the layout and store it in the header.
		uint16_t uVertexLayout{ VERTEX_LAYOUT_DATA_UNDEFINED };
		if (bModelContainsVertexPositions)
			uVertexLayout |= VERTEX_LAYOUT_DATA_POSITION;
		if (bModelContainsVertexNormals)
			uVertexLayout |= VERTEX_LAYOUT_DATA_NORMAL;
		if (bModelContainsVertexTexCoords)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TEXCOORD;
		if (bModelContainsVertexTangents)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TANGENT;
		
		outFileHeader.iVertexLayoutFlags = uVertexLayout;

		// Rewind the file so we can read it through again.
		FileSystem::ReturnFilePointerToHead(&hOBJFile);

		// Geometry data.
		Utl::CVector<CModel_MeshHeader_t>	meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>	meshFaces{};

		// Build out geometry tables.
		Utl::CVector<Vector>	vertexPositions{};
		Utl::CVector<Vector>	vertexNormals{};
		Utl::CVector<Vector2D>	vertexTexCoords{};
		Utl::CVector<Vector>	vertexTangents{};

		Platform::MemorySet(szCurrentLine, 0, sizeof(char) * 2048);
		bytesRead = 0;

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile))
		{
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];

			switch (firstChar)
			{
			case '#':	// Skip comments.
			case 0:		// And blank lines
				break;

			case 'o':	// New geometry object.
			{
				CModel_MeshHeader_t newMesh{};
				meshHeaders.PushBack(newMesh);
				break;
			}

			case 'g':
			{
				// Skip new groups, we'll just put all the geometry in the same mesh.
				continue;
			}

			case 'f':	// Polygon 'face'.
			{
				uint32_t	normalIndices[3];
				uint32_t	positionIndices[3];
				uint32_t	texCoordIndices[3];

				int elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &positionIndices[0], &texCoordIndices[0], &normalIndices[0], &positionIndices[1], &texCoordIndices[1], &normalIndices[1], &positionIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				// Write the elements out to the mesh header.
				CModel_MeshFaceData_t newFace{};
				for (int iVert = 0; iVert < 3; iVert++)
				{
					newFace.iPositionIndices[iVert] = positionIndices[iVert];
					newFace.iNormalIndices[iVert] = normalIndices[iVert];
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert];
				}
				
				size_t iFaceIndex = meshFaces.PushBack(newFace);
				meshHeaders.Back().meshFaceIndices.PushBack(iFaceIndex);
				break;
			}

			case 'u':
				// New Material.
				// TODO: Implement!
				break;

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
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newPosition.x, &newPosition.y, &newPosition.z);
						CAssert(numElements == 3);

						// Add it to the table.
						vertexPositions.PushBack(newPosition);
						break;
					}

					case 'n':	// Vertex normals.
					{
						Vector newNormal{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);
						CAssert(numElements == 3);

						// Add it to the table.
						vertexNormals.PushBack(newNormal);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);
						CAssert(numElements == 2);

						// Add it to the table.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}
				}
				break;
			}

			case 'm':
				LOG_WARN("SerializeOBJMesh() -> IMPLEMENT MATERIAL LIBRARY PROCESSING!");
				break;
				
			default:
				//CAssert(0);
				break;
			}
		}

		// Before writing out geometry to the file, generate tangent data and deduplicate extra data.
		if (!bModelContainsVertexTangents)
		{
			LOG_INFO("Generating tangent data for mesh.");
			GenerateTangentData(vertexPositions.Data(), vertexNormals.Data(), vertexTexCoords.Data(), &vertexTangents, meshFaces.Count(), meshFaces.Data());
		}

		// Write out the file header.
		FileSystem::WriteBytes(&outFileHeader, sizeof(CModel_FileHeader_t), &hNewOutputMesh);

		// Write out geometry headers.
		if (meshHeaders.Count() > 0)
		{
			FileSystem::WriteBytes(meshHeaders.Data(), meshHeaders.Stride() * meshHeaders.Count(), &hNewOutputMesh);
		}
		if (meshFaces.Count() > 0)
		{
			FileSystem::WriteBytes(meshFaces.Data(), meshFaces.Stride() * meshFaces.Count(), &hNewOutputMesh);
		}

		// Write out geometry tables.
		if (vertexPositions.Count() > 0)
		{
			FileSystem::WriteBytes(vertexPositions.Data(), vertexPositions.Stride() * vertexPositions.Count(), &hNewOutputMesh);
		}
		if (vertexNormals.Count() > 0)
		{
			FileSystem::WriteBytes(vertexNormals.Data(), vertexNormals.Stride() * vertexNormals.Count(), &hNewOutputMesh);
		}
		if (vertexTexCoords.Count() > 0)
		{
			FileSystem::WriteBytes(vertexTexCoords.Data(), vertexTexCoords.Stride() * vertexTexCoords.Count(), &hNewOutputMesh);
		}
		if (vertexTangents.Count() > 0)
		{
			FileSystem::WriteBytes(vertexTangents.Data(), vertexTangents.Stride() * vertexTangents.Count(), &hNewOutputMesh);
		}

		// Close the new file.
		FileSystem::CloseFile(&hNewOutputMesh);

		// Close the original source file.
		FileSystem::CloseFile(&hOBJFile);

		return true;
	}

	static void GenerateTangentData(Vector* pPositions, Vector* pNormals, Vector2D* pTexCoords, Utl::CVector<Vector>* pOutTangents, uint64_t iFaceCount, CModel_MeshFaceData_t* pFaces)
	{
		uint64_t invalidTangentCount{ 0 };
		pOutTangents->Resize(iFaceCount);
		for (int iFace = 0; iFace < iFaceCount; iFace++)
		{
			Vector& pPos0 = pPositions[pFaces[iFace].iPositionIndices[0]];
			Vector& pPos1 = pPositions[pFaces[iFace].iPositionIndices[1]];
			Vector& pPos2 = pPositions[pFaces[iFace].iPositionIndices[2]];

			Vector2D& pUV0 = pTexCoords[pFaces[iFace].iTexCoordIndices[0]];
			Vector2D& pUV1 = pTexCoords[pFaces[iFace].iTexCoordIndices[1]];
			Vector2D& pUV2 = pTexCoords[pFaces[iFace].iTexCoordIndices[2]];

			Vector edge1 = pPos1 - pPos0;
			Vector edge2 = pPos2 - pPos0;

			Vector2D deltaUV1 = pUV1 - pUV0;
			Vector2D deltaUV2 = pUV2 - pUV0;

			float flDividend = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
			float f = 1.0f / flDividend;
			CAssert(!(pPos0 == pPos1));
			CAssert(!(pPos1 == pPos2));
			CAssert(!(pPos0 == pPos2));

			if (M_ABS(flDividend) < FLT_EPSILON)
			{
				// Don't average this value with the existing tangent data if it's invalid (It wil just fubar results further.)
				invalidTangentCount++;
				continue;
			}

			Vector vecTangent{};
			vecTangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			vecTangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			vecTangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			// LEFTOFF: Figure out how to organize generated tangent data.
		}

		// Report invalid tangents.
		if (invalidTangentCount > 0)
		{
			LOG_WARN("Mesh contains %d invalid tangents. Lighting will be broken. Please verify that model sources are properly setup.", invalidTangentCount);
		}
	}

	static void DeduplicateGeometry(Vector* pPositions, Vector* pNormals, Vector2D* pTexCoords, CModel_MeshFaceData_t* pFaces)
	{

	}

}	// namespace MeshSerializer
