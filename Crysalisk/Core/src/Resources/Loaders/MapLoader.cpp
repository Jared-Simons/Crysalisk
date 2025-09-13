//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Map resource loading utilities.
//=============================================================================
#include "MapLoader.h"

#include "Core/Assertions.h"
#include "Core/Logging.h"
#include "Platform/FileSystem.h"

namespace MapLoader
{
	// Helper functions.
	static bool ProcessStaticMeshEntry(FileHandle_t& hMapFile, MapResource_t& pOutMapResource);

	bool LoadMapResource(const char* pszMapFilePath, struct MapResource_t& pOutResource)
	{
		FileHandle_t hCMapFile{};

		// Make sure the asset exists first.
		if (!pszMapFilePath || !FileSystem::FileExists(pszMapFilePath))
		{
			LOG_ERROR("Failed to find map file '%s'.", pszMapFilePath);
			return false;
		}

		if (!FileSystem::OpenFile(pszMapFilePath, eFileMode::FILEMODE_READ, false, &hCMapFile))
		{
			LOG_ERROR("Failed to open map file '%s'.", pszMapFilePath);
			return false;
		}

		// Parse the file's contents.
		while (!FileSystem::IsEOF(&hCMapFile))
		{
			char	szCurrentLine[kScratchStringBufferLength]{};
			size_t	currentLineLength{ 0 };
			size_t	bytesRead{ 0 };
			size_t	equalIndex{ 0 };
			char	szKey[500]{};
			char	szValue[500]{};

			FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &hCMapFile);
			currentLineLength = Utl::StringLength(szCurrentLine);

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			equalIndex = Utl::StringIndexOf(szCurrentLine, '=');
			//CAssert(equalIndex != kInvalidID_U32);
			if (equalIndex == kInvalidID_U32)
			{
				continue;
			}
			Platform::MemoryCopy(szKey, szCurrentLine, equalIndex);
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';

			if (Utl::StringCompare(szKey, "map_version"))
			{
				// TODO: Version handling.
			}
			else if (Utl::StringCompare(szKey, "map_name"))
			{
				pOutResource.pszMapName = Utl::StringDuplicate(szValue);
			}
			else if (Utl::StringCompare(szKey, "geometry"))
			{
				if (Utl::StringCompare(szValue, "static_mesh"))
				{
					// MAPTODO: Figure out why the allocator is causing this to crash.
					ProcessStaticMeshEntry(hCMapFile, pOutResource);
				}
			}
		}

		// Remember to close the file.
		FileSystem::CloseFile(&hCMapFile);

		return true;
	}

	static bool ProcessStaticMeshEntry(FileHandle_t& hMapFile, MapResource_t& pOutMapResource)
	{
		if (FileSystem::IsEOF(&hMapFile))
		{
			// This should NEVER happen!
			// This means that the map file contains a static mesh entry that was not properly terminated.
			LOG_WARN("Map file '%s' contains a formatting error! static_mesh entry was not properly termainted!");
			return false;
		}

		char	szCurrentLine[kScratchStringBufferLength]{};
		size_t	iCurrentLineLength{ 0 };
		size_t	bytesRead{ 0 };
		size_t	equalIndex{ 0 };
		char	szKey[64]{};
		char	szValue[64]{};

		// Add a new entry to the resource to write to.
		pOutMapResource.MeshResources.PushBack(MeshLoader::MeshResource_t{});
		MeshLoader::MeshResource_t& newMeshResource = pOutMapResource.MeshResources.Back();

		// Read the next line from where the file left off.
		FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &hMapFile);

		while (szCurrentLine[0] != '}')
		{
			// We can read this again immediately, assuming the first line of this block is just an opening brace.
			FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &hMapFile);
			iCurrentLineLength = Utl::StringLength(szCurrentLine);

			if (iCurrentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line, comment, or whitespace.
				continue;

			if (szCurrentLine[0] == '}')
			{
				// We're done processing this entry.
				return true;
			}

			equalIndex = Utl::StringIndexOf(szCurrentLine, '=');
			//CAssert(equalIndex != kInvalidID_U32);					// MAPTODO: Implement a string utility for checking for whitespace.
			if (equalIndex == kInvalidID_U32)
			{
				continue;
			}

			// Make sure to clear out any existing string data from a previous line read.
			Platform::MemorySet(szKey, 0, 64);
			Platform::MemorySet(szValue, 0, 64);
			
			Platform::MemoryCopy(szKey, &szCurrentLine[1], equalIndex - 1);	// MAPTODO: Using index 1 here to avoid a dynamic alloc in stripping the string.
																		// We REALLY need a utility for string stripping in place...
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';

			// Process mesh properties.
			if (Utl::StringCompare(szKey, "name"))
			{
				// MAPTODO: Currently doing nothing with names.
			}
			else if (Utl::StringCompare(szKey, "model_resource"))
			{
				// Format the path and load up this model resource for use.
				char szFormattedModelPath[512]; // Hopefully this is enough... we're really pushing the stack here!
													// MAPTODO: Find a way to strip the quotes out of a value here on the stack
													// For now, I'm simply going to remove the quotes from the file format, but this should not be necessary!

				Utl::StringFormat_V("Assets/Models/%s.cmodel_c", szFormattedModelPath, szValue);	// MAPTODO: Map the mesh loader take no file extension and have it determine the right method at runtime.

				if (!MeshLoader::LoadMeshResource(szFormattedModelPath, newMeshResource))
				{
					// TODO: Logging.
					return false;
				}
			}
			else if (Utl::StringCompare(szKey, "position"))
			{
				newMeshResource.vecPosition = Utl::String_ToVector3(szValue);
			}
			else if (Utl::StringCompare(szKey, "rotation"))
			{
				newMeshResource.vecEulerAngles = Utl::String_ToVector3(szValue);
			}
			else if (Utl::StringCompare(szKey, "scale"))
			{
				newMeshResource.vecScale = Utl::String_ToVector3(szValue);
			}
			else
			{
				// MAPTODO: Handle other misc mesh properties.
			}
		}

		return true;
	}
} // namespace MapLoader
