//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#include "MeshLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"
#include "Resources/Serializers/MeshSerializer.h"

#include <stdio.h> // For sscanf()

namespace MeshLoader
{
	// Helper functions.
	static bool LoadCModelMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource);

	static bool	LoadOBJMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource);
	static bool LoadOBJMaterialLibrary(const char* pszMtlLibFileName, MeshResource_t& pMeshResource);

	// @Multithreading: This function is a prime candidate for threading.
	//-------------------------------------------------------------------------
	// TODO: Documentation
	//-------------------------------------------------------------------------
	bool LoadMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		if (!FileSystem::FileExists(pszMeshFileName))
		{
			LOG_ERROR("Mesh resource file '%s' could not be found. Mesh resource load failed.", pszMeshFileName);
			return false;
		}

		// Grab the extension from the path.
		char* pszFileExtension = StringGetFilenameExtension(pszMeshFileName);

		if (StringCompare(pszFileExtension, ".cmodel_c"))
		{
			LoadCModelMeshResource(pszMeshFileName, pOutResource);
		}
		else if (StringCompare(pszFileExtension, ".obj"))
		{
			LoadOBJMeshResource(pszMeshFileName, pOutResource);
		}
		else
		{
			// Other file formats not yet supported.
			CAssert(0);
		}

		// Free temp string data.
		StringFree(pszFileExtension);

		return true;
	}

	//-------------------------------------------------------------------------
	// TODO: Documentation
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

		// First up is the magic number.
		uint32_t* pMagicNumber = (uint32_t*)modelBytes.Data();
		if (*pMagicNumber != MeshSerializer::kModelResourceMagicNumber)
		{
			LOG_ERROR("'%s' mesh does not contain kModelResourceMagicNumber, resource reading failed.");
			return false;
		}

		// File header.
		MeshSerializer::CModel_FileHeader_t* pFileHeader = (MeshSerializer::CModel_FileHeader_t*)((uint8_t*)pMagicNumber + 4);

		FileSystem::CloseFile(&hCModelFile);

		return true;
	}

	//-------------------------------------------------------------------------
	// TODO: Documentation
	//-------------------------------------------------------------------------
	static bool	LoadOBJMeshResource(const char* pszMeshFileName, MeshResource_t& pOutResource)
	{
		FileHandle_t hObjFile{};

		if (!FileSystem::OpenFile(pszMeshFileName, eFileMode::FILEMODE_READ, false, &hObjFile))
		{
			return false;
		}

		// Some helper variables.
		MeshResource_t::GeometryObject_t*		pCurrentObject{ nullptr };
		MeshResource_t::GeometryGroup_t*		pCurrentGroup{ nullptr };
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

			FileSystem::ReadLine(szCurrentLine, 1000, &bytesRead, &hObjFile);
			currentLineLength = StringLength(szCurrentLine);

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			// The type of data we're processing depends on the first 1-2 characters of the line.
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];

			switch (firstChar)
			{
			case 'o':	// Objects
				pOutResource.pObjects.PushBack(new MeshResource_t::GeometryObject_t);
				pCurrentObject = pOutResource.pObjects.Back();
				pCurrentObject->pszDebugName = StringDuplicate(&szCurrentLine[2]);
				nObjectsProcessed++;
				break;

			case 'g':	// Groups
				if (!pCurrentObject)
				{
					// If we recieved an OBJ without objects, then create one for everything.
					pOutResource.pObjects.PushBack(new MeshLoader::MeshResource_t::GeometryObject_t);
					pCurrentObject = pOutResource.pObjects.Back();
				}
				CAssert(pCurrentObject);

				pCurrentObject->pGroups.PushBack(new MeshResource_t::GeometryGroup_t);
				pCurrentGroup = pCurrentObject->pGroups.Back();
				pCurrentGroup->pszDebugName = StringDuplicate(&szCurrentLine[2]);
				break;

			case 'm':
				// Material library file.

				// Only load this once!
				// TODO: This only accounts for a single material lib per .obj file!
				if (!bLoadedMaterialLib)
				{
					pszMtlPath = StringDuplicate(pszMeshFileName);
					CAssert(pszMtlPath);
					mtlPathLength = StringLength(pszMtlPath);
					pszMtlPath[mtlPathLength - 4] = '\0';

					pszFormattedMtlPath = StringConcat(pszMtlPath, ".mtl");

					LoadOBJMaterialLibrary(pszFormattedMtlPath, pOutResource);

					// Free the temp string data.
					if (pszFormattedMtlPath)
						StringFree(pszFormattedMtlPath);
					if (pszMtlPath)
						StringFree(pszMtlPath);

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
						int numElements = sscanf(&szCurrentLine[2], "%f %f %f", &newVertex.x, &newVertex.y, &newVertex.z);

						CAssert(numElements == 3);
						// Add it to the array.
						vertexPositions.PushBack(newVertex);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						int numElements = sscanf(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);

						CAssert(numElements == 2);
						// Add it to the array.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}

					case 'n':	// Vertex normals.
					{
						Vector3D newNormal{};
						int numElements = sscanf(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);

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

				int elements = sscanf(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndices[0], &texCoordIndices[0], &normalIndices[0], &vertexIndices[1], &texCoordIndices[1], &normalIndices[1], &vertexIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				Vertex3D newVertices[3];

				for (int idx = 0; idx < 3; idx++)
				{
					// NOTE: Obj files are 1-indexed so we need to bump down one index to align properly.
					newVertices[idx].Pos = vertexPositions[vertexIndices[idx] - 1];
					newVertices[idx].TexCoord = vertexTexCoords[texCoordIndices[idx] - 1];
					newVertices[idx].Normal = vertexNormals[normalIndices[idx] - 1];

					CAssert(pCurrentGroup && pCurrentObject);

					pCurrentGroup->Vertices.PushBack(newVertices[idx]);
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
					pCurrentObject->pGroups.PushBack(new MeshLoader::MeshResource_t::GeometryGroup_t);
					pCurrentGroup = pCurrentObject->pGroups.Back();
				}
				else if (bMaterialDefinedBeforeNewGroup)
				{
					// We need to create a new group to work with here.
					pCurrentObject->pGroups.PushBack(new MeshLoader::MeshResource_t::GeometryGroup_t);
					pCurrentGroup = pCurrentObject->pGroups.Back();
				}

				CAssert(pCurrentGroup);
				size_t spaceIdx = StringIndexOf(szCurrentLine, ' ');
				pCurrentGroup->pszMaterialName = StringDuplicate(&szCurrentLine[spaceIdx + 1]);
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
		size_t dotIndex = StringIndexOf(pszMeshFileName, '.');
		char* pTempNameBuffer = StringDuplicate_N(pszMeshFileName, dotIndex);
		MeshSerializer::SerializeMeshResource(pTempNameBuffer, pOutResource);
		StringFree(pTempNameBuffer);

		return true;
	}

	//-------------------------------------------------------------------------
	// TODO: Documentation
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
			spaceIndex = StringIndexOf(szCurrentLine, ' ');
			pFirstChar = szCurrentLine[0];
			pSecondChar = szCurrentLine[1];

			if (spaceIndex != kInvalidID_U32)
			{
				// Just in case the newline starts with a '\t'.
				int startIndex = (szCurrentLine[0] == '\t') ? 1 : 0;
				pKey = StringDuplicate_N(&szCurrentLine[startIndex], spaceIndex);
				pValue = StringDuplicate(&szCurrentLine[spaceIndex + 1]);
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
					currentMaterialResource.pszMaterialName = StringDuplicate(&szCurrentLine[spaceIndex + 1]);

					break;
				}

				case 's':	// Specular exponent.
				{
					currentMaterialResource.pszCustomKeys.PushBack(StringDuplicate("Shininess"));
					currentMaterialResource.pszCustomValues.PushBack(StringDuplicate(&szCurrentLine[spaceIndex]));
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
					// TODO: Clean up this string parsing some...
					// Diffuse map.
					if (StringCompare(pKey, "map_Kd ") || StringCompare(pKey, "map_Ka "))
					{
						//CAssert(currLine != 50);
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t diffuseMap{};
						diffuseMap.eType = MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE;
						diffuseMap.pszTextureName = StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(diffuseMap);
					}
					// Specular map.
					else if (StringCompare(pKey, "map_Ks") || StringCompare(pKey, "map_Ks "))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t specularMap{};
						specularMap.eType = MaterialLoader::MaterialResource_t::eTextureType::SPECULAR;
						specularMap.pszTextureName = StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(specularMap);
					}
					// Normal map.
					else if (StringCompare(pKey, "map_Disp") || StringCompare(pKey, "map_Disp ") || StringCompare(pKey, "map_bump") || StringCompare(pKey, "map_bump "))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t normalMap{};
						normalMap.eType = MaterialLoader::MaterialResource_t::eTextureType::NORMAL;
						normalMap.pszTextureName = StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(normalMap);
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
				StringFree(pKey);
				StringFree(pValue);
			}

			currLine++;
		}

		// Flush the last config to the resource, it won't be picked up by the above code.
		pMeshResource.pMaterials.PushBack(currentMaterialResource);

		FileSystem::CloseFile(&hMtlFile);

		return true;
	}
}