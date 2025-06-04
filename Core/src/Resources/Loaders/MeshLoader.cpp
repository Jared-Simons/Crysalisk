//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#include "MeshLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"

#include <stdio.h> // For sscanf()

namespace MeshLoader
{
	// Helper functions.
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

		if (StringCompare(pszFileExtension, ".obj"))
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
				CAssert(pCurrentObject);
				pCurrentObject->pGroups.PushBack(new MeshResource_t::GeometryGroup_t);
				pCurrentGroup = pCurrentObject->pGroups.Back();
				pCurrentGroup->pszDebugName = StringDuplicate(&szCurrentLine[2]);
				break;

			case 'm':
				// Material library file.
				// TODO: Implement.
				pszMtlPath = StringDuplicate(pszMeshFileName);
				CAssert(pszMtlPath);
				mtlPathLength = StringLength(pszMtlPath);
				pszMtlPath[mtlPathLength - 4] = '\0';

				pszFormattedMtlPath = StringConcat(pszMtlPath, ".mtl");

				//LoadOBJMaterialLibrary(pszFormattedMtlPath, pOutResource);

				// Free the temp string data.
				if (pszFormattedMtlPath)
					StringFree(pszFormattedMtlPath);
				if (pszMtlPath)
					StringFree(pszMtlPath);

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

			default:
				//CAssert(0);
				break;
			}
		}

		FileSystem::CloseFile(&hObjFile);

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
			CAssert(currLine != 58);

			if (spaceIndex != kInvalidID_U32)
			{
				// Just in case the newline starts with a '\t'.
				int startIndex = (szCurrentLine[0] == '\t') ? 1 : 0;
				pKey = StringDuplicate_N(&szCurrentLine[startIndex], spaceIndex - 1);
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
					// Diffuse map.
					if (StringCompare(pKey, "map_Kd") || StringCompare(pKey, "map_Ka"))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t diffuseMap{};
						diffuseMap.eType = MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE;
						diffuseMap.pszTextureName = StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(diffuseMap);
					}
					// Specular map.
					else if (StringCompare(pKey, "map_Ks"))
					{
						MaterialLoader::MaterialResource_t::eTextureMapConfig_t specularMap{};
						specularMap.eType = MaterialLoader::MaterialResource_t::eTextureType::SPECULAR;
						specularMap.pszTextureName = StringDuplicate(pValue);
						currentMaterialResource.textureMapConfigs.PushBack(specularMap);
					}
					// Normal map.
					else if (StringCompare(pKey, "map_Disp") || StringCompare(pKey, "map_bump"))
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

		FileSystem::CloseFile(&hMtlFile);

		return true;
	}
}