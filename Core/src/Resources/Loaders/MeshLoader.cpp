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
	static bool LoadOBJMaterialLibrary(const char* pszMtlLibFileName);

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

		// Parse the file's contents.
		while (!FileSystem::IsEOF(&hObjFile) && nObjectsProcessed < 10)
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
				pCurrentGroup->DebugName = StringDuplicate(&szCurrentLine[2]);
				break;

			case 'm':
				// Material library file.
				// TODO: Implement.
				LoadOBJMaterialLibrary(nullptr);
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
				uint32_t	notmalIndices[3];

				int elements = sscanf(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndices[0], &texCoordIndices[0], &notmalIndices[0], &vertexIndices[1], &texCoordIndices[1], &notmalIndices[1], &vertexIndices[2], &texCoordIndices[2], &notmalIndices[2]);

				CAssert(elements == 9);

				Vertex3D newVertices[3];

				for (int idx = 0; idx < 3; idx++)
				{
					newVertices[idx].Pos = vertexPositions[vertexIndices[idx]];
					newVertices[idx].TexCoord = vertexTexCoords[texCoordIndices[idx]];
					newVertices[idx].Normal = vertexNormals[notmalIndices[idx]];

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
	static bool LoadOBJMaterialLibrary(const char* pszMtlLibFileName)
	{
		LOG_WARN("Implement Material Loader for OBJ Material Library!");
		return true;
	}
}