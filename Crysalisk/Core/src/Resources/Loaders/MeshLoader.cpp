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

	static bool	LoadOBJMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource);
	static bool LoadOBJMaterialLibrary(const char* pszMtlLibFileName, MeshResource_t& pMeshResource);
	static void DeduplicateGeometry(MeshResource_t::GeometryObject_t* pGeometryConfig);
	static void GenerateTangentData(MeshResource_t::GeometryObject_t* pConfig);

	// @Multithreading: This function is a prime candidate for threading.
	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing mesh asset from disk.
	// 
	// Input:	pszMeshFileName - The path to the image to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		if (!FileSystem::FileExists(pszMeshFileName))
		{
			LOG_ERROR("Mesh resource file '%s' could not be found. Mesh resource load failed.", pszMeshFileName);
			return false;
		}

		// Grab the extension from the path.
		char* pszFileExtension = Utl::StringGetFilenameExtension(pszMeshFileName);

		if (Utl::StringCompare(pszFileExtension, ".cmodel_c"))
		{
			LoadCModelMeshResource(pszMeshFileName, pOutResource);
		}
		else if (Utl::StringCompare(pszFileExtension, ".obj"))
		{
			LoadOBJMeshResource(pszMeshFileName, pOutResource);
		}
		else
		{
			// Other file formats not yet supported.
			CAssert(0);
		}

		// Free temp string data.
		Utl::StringFree(pszFileExtension);

		return true;
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

		void*		pDataPtr = modelBytes.Data();
		uint64_t	currOffset{ 0 };

		// First up is the magic number.
		uint32_t* pMagicNumber = (uint32_t*)modelBytes.Data();
		if (*pMagicNumber != MeshSerializer::kModelResourceMagicNumber)
		{
			LOG_ERROR("'%s' mesh does not contain kModelResourceMagicNumber, resource reading failed.");
			return false;
		}

		// Advance to the header.
		currOffset += sizeof(uint32_t);

		// Setup a new mesh object.
		pOutResource.pObjects.PushBack(new MeshResource_t::GeometryGroup_t);

		// File header.
		MeshSerializer::CModel_FileHeader_t* pFileHeader = (MeshSerializer::CModel_FileHeader_t*)((uint8_t*)pDataPtr + currOffset);

		// Advance to the mesh data.
		currOffset += sizeof(MeshSerializer::CModel_FileHeader_t);

		// Process mesh data.
		for (int meshIdx = 0; meshIdx < pFileHeader->iSubmeshDataSize; meshIdx++)
		{
			// Allocate a new geometry group.
			pOutResource.pObjects[0]->pGroups.PushBack(new MeshResource_t::GeometryObject_t);

			MeshSerializer::CModel_SubmeshHeader_t* pCurrentMesh = (MeshSerializer::CModel_SubmeshHeader_t*)((uint8_t*)pDataPtr + currOffset);
			CAssert(pCurrentMesh->iSubmeshMagic == 0xBC);

			// Next up is the mesh name.
			currOffset += sizeof(MeshSerializer::CModel_SubmeshHeader_t);
			pOutResource.pObjects.Back()->pGroups[meshIdx]->pszDebugName = new char[pCurrentMesh->iSubmeshNameLength + 1];
			Platform::MemoryCopy(pOutResource.pObjects.Back()->pGroups[meshIdx]->pszDebugName,
								((uint8_t*)pDataPtr + currOffset),
								pCurrentMesh->iSubmeshNameLength);

			// Material name is next.
			currOffset += pCurrentMesh->iSubmeshNameLength;
			pOutResource.pObjects.Back()->pGroups[meshIdx]->pszMaterialName = new char[pCurrentMesh->iMaterialNameLength + 1];
			Platform::MemoryCopy(pOutResource.pObjects.Back()->pGroups[meshIdx]->pszMaterialName,
								((uint8_t*)pDataPtr + currOffset),
								pCurrentMesh->iMaterialNameLength);

			// Get at the vertex data.
			currOffset += pCurrentMesh->iMaterialNameLength;
			Vertex3D* pVertexTable = (Vertex3D*)((uint8_t*)pDataPtr + currOffset);

			// Vertex data.
			for (int vertIdx = 0; vertIdx < pCurrentMesh->iVertexCount; vertIdx++)
			{
				Vertex3D* newVert = &pVertexTable[vertIdx];
				pOutResource.pObjects.Back()->pGroups[meshIdx]->Vertices.PushBack(*newVert);
			}

			// Get at the index data.
			currOffset += pCurrentMesh->iVertexCount * pCurrentMesh->iVertexStride;
			uint32_t* pIndexTable = (uint32_t*)((uint8_t*)pDataPtr + currOffset);

			// Index data.
			for (int idx = 0; idx < pCurrentMesh->iIndexCount; idx++)
			{
				uint32_t* newIdx = &pIndexTable[idx];
				pOutResource.pObjects.Back()->pGroups[meshIdx]->Indices.PushBack(*newIdx);
			}

			// Get to the next submesh.
			currOffset += pCurrentMesh->iIndexCount * pCurrentMesh->iIndexStride;
		}

		FileSystem::CloseFile(&hCModelFile);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to an .OBJ model asset at the given path.
	// 
	// Input:	pszMeshFileName - The path to the model to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	static bool	LoadOBJMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		FileHandle_t hObjFile{};

		if (!FileSystem::OpenFile(pszMeshFileName, eFileMode::FILEMODE_READ, false, &hObjFile))
		{
			return false;
		}

		// Some helper variables.
		MeshResource_t::GeometryGroup_t*		pCurrentObject{ nullptr };
		MeshResource_t::GeometryObject_t*		pCurrentGroup{ nullptr };
		int										nObjectsProcessed{ 0 };
		Utl::CVector<Vector3D>					vertexPositions{};
		Utl::CVector<Vector2D>					vertexTexCoords{};
		Utl::CVector<Vector3D>					vertexNormals{};

		char* pszMtlPath = nullptr;
		size_t mtlPathLength = 0;
		char* pszFormattedMtlPath = nullptr;

		bool	bMaterialDefinedBeforeNewGroup = false;
		bool	bLoadedMaterialLib = false;

		// Parse the file's contents.
		while (!FileSystem::IsEOF(&hObjFile) && nObjectsProcessed < 1024)
		{
			char	szCurrentLine[1000];
			size_t	currentLineLength{ 0 };
			size_t	bytesRead{ 0 };
			char	firstChar{};
			char	secondChar{};
			static bool bTestBool = false;
			static int iTestInt = 0;

			FileSystem::ReadLine(szCurrentLine, 1000, &bytesRead, &hObjFile);
			currentLineLength = Utl::StringLength(szCurrentLine);

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			// The type of data we're processing depends on the first 1-2 characters of the line.
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];

			switch (firstChar)
			{
			case 'o':	// Objects
				pOutResource.pObjects.PushBack(new MeshResource_t::GeometryGroup_t);
				pCurrentObject = pOutResource.pObjects.Back();
				pCurrentObject->pszDebugName = Utl::StringDuplicate(&szCurrentLine[2]);
				nObjectsProcessed++;
				break;

			case 'g':	// Groups
				if (!pCurrentObject)
				{
					// If we recieved an OBJ without objects, then create one for everything.
					pOutResource.pObjects.PushBack(new MeshLoader::MeshResource_t::GeometryGroup_t);
					pCurrentObject = pOutResource.pObjects.Back();
				}
				CAssert(pCurrentObject);

				// Check for duplicate vertices.
				if (pCurrentGroup && iTestInt < 5)
				{
					DeduplicateGeometry(pCurrentGroup);

					// Generate our tangent data here as well.
					GenerateTangentData(pCurrentGroup);
					bTestBool = true;
					//iTestInt++;
				}

				pCurrentObject->pGroups.PushBack(new MeshResource_t::GeometryObject_t);
				pCurrentGroup = pCurrentObject->pGroups.Back();
				if (pCurrentGroup)
					pCurrentGroup->pszDebugName = Utl::StringDuplicate(&szCurrentLine[2]);
				break;

			case 'm':
				// Material library file.

				// Only load this once!
				// NOTE: This only accounts for a single material lib per .obj file.
				if (!bLoadedMaterialLib)
				{
					pszMtlPath = Utl::StringDuplicate(pszMeshFileName);
					CAssert(pszMtlPath);
					mtlPathLength = Utl::StringLength(pszMtlPath);
					pszMtlPath[mtlPathLength - 4] = '\0';

					pszFormattedMtlPath = Utl::StringConcat(pszMtlPath, ".mtl");

					LoadOBJMaterialLibrary(pszFormattedMtlPath, pOutResource);

					// Free the temp string data.
					if (pszFormattedMtlPath)
						Utl::StringFree(pszFormattedMtlPath);
					if (pszMtlPath)
						Utl::StringFree(pszMtlPath);

					bLoadedMaterialLib = true;
				}

				break;

			case 'v':
			{
				switch (secondChar)
				{
					case ' ':	// Vertex position.
					{
						Vector3D newVertex{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newVertex.x, &newVertex.y, &newVertex.z);

						CAssert(numElements == 3);
						// Add it to the array.
						vertexPositions.PushBack(newVertex);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);

						CAssert(numElements == 2);
						// Add it to the array.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}

					case 'n':	// Vertex normals.
					{
						Vector3D newNormal{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);

						CAssert(numElements == 3);
						// Add it to the array.
						vertexNormals.PushBack(newNormal);
						break;
					}
				}
				break;
			}

			case 'f':	// Polygon 'face'.
			{
				uint32_t	vertexIndices[3];
				uint32_t	texCoordIndices[3];
				uint32_t	normalIndices[3];

				int elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndices[0], &texCoordIndices[0], &normalIndices[0], &vertexIndices[1], &texCoordIndices[1], &normalIndices[1], &vertexIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				Vertex3D newVertices[3];

				for (int idx = 0; idx < 3; idx++)
				{
					// NOTE: Obj files are 1-indexed so we need to bump down one index to align properly.
					newVertices[idx].Pos = vertexPositions[vertexIndices[idx] - 1];
					newVertices[idx].TexCoord = vertexTexCoords[texCoordIndices[idx] - 1];
					newVertices[idx].Normal = vertexNormals[normalIndices[idx] - 1];

					CAssert(pCurrentGroup && pCurrentObject);

					size_t currIdx = pCurrentGroup->Vertices.PushBack(newVertices[idx]);
				}

				break;
			}

			case 'u':	// 'usemtl' Use material.
			{
				CAssert(pCurrentObject);
				if (!pCurrentGroup)
				{
					// It is possible that the material name was declared before the group was...
					bMaterialDefinedBeforeNewGroup = true;

					// Create a new group.
					pCurrentObject->pGroups.PushBack(new MeshLoader::MeshResource_t::GeometryObject_t);
					pCurrentGroup = pCurrentObject->pGroups.Back();
				}
				else if (bMaterialDefinedBeforeNewGroup)
				{
					// We need to create a new group to work with here.
					pCurrentObject->pGroups.PushBack(new MeshLoader::MeshResource_t::GeometryObject_t);
					pCurrentGroup = pCurrentObject->pGroups.Back();
				}

				CAssert(pCurrentGroup);
				size_t spaceIdx = Utl::StringIndexOf(szCurrentLine, ' ');
				pCurrentGroup->pszMaterialName = Utl::StringDuplicate(&szCurrentLine[spaceIdx + 1]);
				break;
			}

			case 's':	// 's' Smoothing group -- currently unused.
			{
				continue;
			}

			default:
				CAssert(0);
				break;
			}
		}

		FileSystem::CloseFile(&hObjFile);

		// Write the resource out to our engine-specific format.

		// Write it without the original extension
		size_t dotIndex = Utl::StringIndexOf(pszMeshFileName, '.');
		char* pTempNameBuffer = Utl::StringDuplicate_N(pszMeshFileName, dotIndex);
		MeshSerializer::SerializeMeshResource(pTempNameBuffer, pOutResource);
		for (int idx = 0; idx < pOutResource.pMaterials.Count(); idx++)
		{
			MaterialSerializer::SerializeMaterialResources(pOutResource.pMaterials[idx].pszMaterialName, pOutResource.pMaterials[idx]);
		}

		Utl::StringFree(pTempNameBuffer);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load the contents of an .OBJ's material library into
	//				a MaterialResource_t table associated with the mesh resource.
	// 
	// Input:	pszMtlLibFileName - The path to the material library to be parsed.
	//			pOutResource - The mesh associated with this material library.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	static bool LoadOBJMaterialLibrary(const char* pszMtlLibFileName, MeshResource_t& pMeshResource)
	{
		LOG_DEBUG("Importing obj material library file '%s'.", pszMtlLibFileName);

		// Make sure the file exists first!
		if (!FileSystem::FileExists(pszMtlLibFileName))
		{
			LOG_WARN("Failed to find obj material library file '%s'.", pszMtlLibFileName);
			return false;
		}

		FileHandle_t hMtlFile{};
		if (!FileSystem::OpenFile(pszMtlLibFileName, eFileMode::FILEMODE_READ, false, &hMtlFile))
		{
			return false;
		}

		char szCurrentLine[1000]{};
		char pFirstChar{};
		char pSecondChar{};
		size_t currLine{ 1 };
		size_t bytesRead{ 0 };
		size_t spaceIndex{ 0 };
		int groups{ 0 };
		char* pKey;
		char* pValue;

		MaterialLoader::MaterialResource_t currentMaterialResource{};

		while (!FileSystem::IsEOF(&hMtlFile))
		{
			FileSystem::ReadLine(szCurrentLine, 1000, &bytesRead, &hMtlFile);

			szCurrentLine[bytesRead] = 0;
			spaceIndex = Utl::StringIndexOf(szCurrentLine, ' ');
			pFirstChar = szCurrentLine[0];
			pSecondChar = szCurrentLine[1];

			if (spaceIndex != kInvalidID_U32)
			{
				// Just in case the newline starts with a '\t'.
				int startIndex = (szCurrentLine[0] == '\t') ? 1 : 0;
				pKey = Utl::StringDuplicate_N(&szCurrentLine[startIndex], spaceIndex);
				pValue = Utl::StringDuplicate(&szCurrentLine[spaceIndex + 1]);
			}

			if (szCurrentLine[0] == '\t')
			{
				pFirstChar = szCurrentLine[1];
				pSecondChar = szCurrentLine[2];
			}

			switch (pFirstChar)
			{
			case '#':	// Skip comments.
			case 0:		// And blank lines
				break;

			case 'n':
			case 'N':
			{
				switch (pSecondChar)
				{
				case 'e':
				{
					if (currentMaterialResource.pszMaterialName != nullptr)
					{
						// A config currently exists, push it to the array.
						pMeshResource.pMaterials.PushBack(currentMaterialResource);

						// Clear any leftover data.
						//pMeshResource.pMaterials.Clear();
						currentMaterialResource.~MaterialResource_t();
					}
					//LOG_INFO("New Material '%s'.", &szCurrentLine[spaceIndex]);
					currentMaterialResource.pszMaterialName = Utl::StringDuplicate(&szCurrentLine[spaceIndex + 1]);

					break;
				}

				case 's':	// Specular exponent.
				{
					currentMaterialResource.pszCustomKeys.PushBack(Utl::StringDuplicate("shininess"));
					currentMaterialResource.pszCustomValues.PushBack(Utl::StringDuplicate(&szCurrentLine[spaceIndex + 1]));
					break;
				}
				}
				break;
			}

			case 'm':
			{
				switch (pSecondChar)
				{
				case 'a':
				{
					char* pszTrimmedMapName = Utl::StringTrim(pKey);
					// Diffuse map.
					if (Utl::StringCompare(pszTrimmedMapName, "map_Kd"))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t diffuseMap{};
						diffuseMap.eType = MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE;
						diffuseMap.pszTextureName = Utl::StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(diffuseMap);
					}
					// Specular map.
					else if (Utl::StringCompare(pszTrimmedMapName, "map_Ks"))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t specularMap{};
						specularMap.eType = MaterialLoader::MaterialResource_t::eTextureType::SPECULAR;
						specularMap.pszTextureName = Utl::StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(specularMap);
					}
					// Normal map.
					else if (Utl::StringCompare(pszTrimmedMapName, "map_Disp") || Utl::StringCompare(pszTrimmedMapName, "map_bump"))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t normalMap{};
						normalMap.eType = MaterialLoader::MaterialResource_t::eTextureType::NORMAL;
						normalMap.pszTextureName = Utl::StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(normalMap);
					}
					if (pszTrimmedMapName)
					{
						Utl::StringFree(pszTrimmedMapName);
						pszTrimmedMapName = nullptr;
					}

					break;
				}
				break;
				}

				break;
			}

			case 'K':
			{
				switch (pSecondChar)
				{
				case 'd':
				{
					//sscanf(szCurrentLine, "Kd %f %f %f\n", &currentMaterialConfig.vecDiffuseColor.x, &currentMaterialConfig.vecDiffuseColor.y, &currentMaterialConfig.vecDiffuseColor.z);
					break;
				}
				}

				break;
			}

			default:
				break;
			}

			if (spaceIndex != kInvalidID_U32)
			{
				Utl::StringFree(pKey);
				Utl::StringFree(pValue);
			}

			currLine++;
		}

		// Flush the last config to the resource, it won't be picked up by the above code.
		pMeshResource.pMaterials.PushBack(currentMaterialResource);

		FileSystem::CloseFile(&hMtlFile);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Given a geometry object, this method attempts to find duplicate vertices
	//				and removes them, as well as setting up a proper index buffer.
	// 
	// Input:	pGeometryConfig - A piece of geometry to be processed.
	// Output:	none.
	//-------------------------------------------------------------------------
	static void DeduplicateGeometry(MeshResource_t::GeometryObject_t* pGeometryConfig)
	{
		size_t					totalDuplicateVerts{ 0 };
		size_t					originalVertCount{ pGeometryConfig->Vertices.Count() };
		bool					bUniqueVert{ true };
		Utl::CVector<Vertex3D>	uniqueVerts(pGeometryConfig->Vertices.Count());

		for (int outerIdx = 0; outerIdx < pGeometryConfig->Vertices.Count(); outerIdx++)
		{
			bUniqueVert = true;
			for (int innerIdx = 0; innerIdx < uniqueVerts.Count(); innerIdx++)
			{
				if (pGeometryConfig->Vertices[outerIdx] == uniqueVerts[innerIdx])
				{
					// This vertex already exists in the uniqueVerts array, just use this index.
					bUniqueVert = false;
					totalDuplicateVerts++;
					pGeometryConfig->Indices.PushBack(innerIdx);
					break;
				}
			}

			if (bUniqueVert)
			{
				// Add this vertex over to the unique array.
				uint32_t currIdx = (uint32_t)uniqueVerts.PushBack(pGeometryConfig->Vertices[outerIdx]);
				pGeometryConfig->Indices.PushBack(currIdx);
			}
		}

		// Kill the old vertex data and save off the unique verts.
		pGeometryConfig->Vertices.Clear();
		pGeometryConfig->Vertices = uniqueVerts;

		CAssert(pGeometryConfig->Vertices.Count() + totalDuplicateVerts == originalVertCount);
		CAssert(pGeometryConfig->Indices.Count() == originalVertCount);

 		LOG_DEBUG("Geometry Deduplication '%s': %u/%u", pGeometryConfig->pszDebugName, originalVertCount, pGeometryConfig->Vertices.Count());
	}

	//-------------------------------------------------------------------------
	// Purpose: Given a geometry object, this method attempts to generate tangent data
	//				required for proper lighting calculations.
	// 
	// Input:	pGeometryConfig - A piece of geometry to be processed.
	// Output:	none.
	//-------------------------------------------------------------------------
	static void GenerateTangentData(MeshResource_t::GeometryObject_t* pConfig)
	{
		size_t invalidTangentCount{ 0 };
		for (int idx = 0; idx < pConfig->Indices.Count(); idx += 3)
		{
			Vertex3D& vert0 = pConfig->Vertices[pConfig->Indices[idx]];
			Vertex3D& vert1 = pConfig->Vertices[pConfig->Indices[idx + 1]];
			Vertex3D& vert2 = pConfig->Vertices[pConfig->Indices[idx + 2]];

			Vector3D edge1 = vert1.Pos - vert0.Pos;
			Vector3D edge2 = vert2.Pos - vert0.Pos;

			Vector2D deltaUV1 = vert1.TexCoord - vert0.TexCoord;
			Vector2D deltaUV2 = vert2.TexCoord - vert0.TexCoord;

			float dividend = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
			float f = 1.0f / dividend;
			CAssert(!(vert0 == vert1));
			CAssert(!(vert1 == vert2));
			CAssert(!(vert0 == vert2));
			if (M_ABS(dividend) < kVectorElementEpsilon)
			{
				// Don't average this value with the existing tangent data if it's invalid (It wil just fubar results further.)
				invalidTangentCount++;
				continue;
			}

			Vector3D vecTangent{};
			vecTangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			vecTangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			vecTangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			vert0.Tangent += vecTangent;
			vert1.Tangent += vecTangent;
			vert2.Tangent += vecTangent;
		}

		// Make sure to normalize all the new tangent data.
		for (int idx = 0; idx < pConfig->Vertices.Count(); idx++)
		{
			pConfig->Vertices[idx].Tangent.NormalizeInPlace();
		}

		// Report invalid tangents.
		if (invalidTangentCount > 0)
		{
			LOG_WARN("GenerateTangentData(): Submesh '%s' contains %d invalid tangents. Lighting will be broken. Please verify that model sources are properly setup.", pConfig->pszDebugName, invalidTangentCount);
		}
	}
}