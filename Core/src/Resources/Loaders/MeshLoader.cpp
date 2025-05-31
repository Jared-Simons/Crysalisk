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
		MeshResource_t::GeometryObject_t*	pCurrentObject{ nullptr };
		int										nObjectsProcessed{ 0 };

		// Parse the file's contents.
		while (!FileSystem::IsEOF(&hObjFile) && nObjectsProcessed < 2)
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

			if (firstChar == 'o')	// A new object.
			{
				size_t newObjectIdx = pOutResource.Objects.PushBack(new MeshResource_t::GeometryObject_t());
				pCurrentObject = &pOutResource.Objects[newObjectIdx];

				// Store off the name of the group (this is purely for debugging purposes).
				pCurrentObject->DebugName = StringDuplicate(&szCurrentLine[2]);
				nObjectsProcessed++;

				//CAssert(0);
			}
			else if (firstChar == 'v')	// Some form of vertex data, need to check the second char to be sure.
			{
				if (secondChar == ' ')		// Vertex positions.
				{
					Vector3D vecNewPosition{};
					int numElementsFound = sscanf(&szCurrentLine[2], "%f %f %f", &vecNewPosition.x, &vecNewPosition.y, &vecNewPosition.z);
					CAssert(numElementsFound == 3);

					// Add it to the array!
					pOutResource.Positions.PushBack(vecNewPosition);
				}
				else if (secondChar == 't')	// Vertex texture coordinate.
				{
					Vector2D vecNewTexCoord{};
					int numElementsFound = sscanf(&szCurrentLine[2], "%f %f", &vecNewTexCoord.x, &vecNewTexCoord.y);
					CAssert(numElementsFound == 2);

					// Add it to the array!
					pOutResource.TexCoords.PushBack(vecNewTexCoord);
				}
				else if (secondChar == 'n')
				{
					Vector3D vecNewNormal{};
					int numElementsFound = sscanf(&szCurrentLine[2], "%f %f %f", &vecNewNormal.x, &vecNewNormal.y, &vecNewNormal.z);
					CAssert(numElementsFound == 3);

					// Add it to the array!
					pOutResource.Normals.PushBack(vecNewNormal);
				}
				else
				{
					// Should be no other options here.
					CAssert(0);
				}
			}
			else if (firstChar == 'm')
			{
				// Material library file.
				// TODO: Implement.
				LoadOBJMaterialLibrary(nullptr);
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