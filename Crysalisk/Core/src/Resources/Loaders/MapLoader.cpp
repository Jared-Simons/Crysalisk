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
	static bool ProcessLightEntity(FileHandle_t& hMapFile, MapResource_t::eLightType type, MapResource_t& pOutMapResource);

	bool LoadMapResource(const char* pszMapFilePath, MapResource_t& pOutResource)
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
				// Note: Version handling.
				pOutResource.iMapVersion = Utl::String_ATOI(szValue);
				LOG_DEBUG("Map version detected: %d", pOutResource.iMapVersion);
			}
			else if (Utl::StringCompare(szKey, "map_name"))
			{
				pOutResource.pszMapName = Utl::StringDuplicate(szValue);
			}
			else if (Utl::StringCompare(szKey, "geometry"))
			{
				if (Utl::StringCompare(szValue, "static_mesh"))
				{
					ProcessStaticMeshEntry(hCMapFile, pOutResource);
				}
			}
			else if (Utl::StringCompare(szKey, "entity"))
			{
				if (Utl::StringCompare(szValue, "light_environment"))
				{
					ProcessLightEntity(hCMapFile, MapResource_t::eLightType::DIRECTIONAL, pOutResource);
				}
				else if (Utl::StringCompare(szValue, "light_point"))
				{
					ProcessLightEntity(hCMapFile, MapResource_t::eLightType::POINT, pOutResource);
				}
				else if (Utl::StringCompare(szValue, "light_spot"))
				{
					ProcessLightEntity(hCMapFile, MapResource_t::eLightType::SPOT, pOutResource);
				}
				else
				{
					LOG_ERROR("Undefined Light entry found: %s", szCurrentLine);
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
			LOG_WARN("Map file '%s' contains a formatting error! static_mesh entry was not properly termainted!", pOutMapResource.pszMapName);
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
		pOutMapResource.MeshResources_NEW.PushBack(MapResource_t::MeshDeclaration_t{});
		MapResource_t::MeshDeclaration_t& newMeshResource_NEW = pOutMapResource.MeshResources_NEW.Back();

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
			if (equalIndex == kInvalidID_U32)
			{
				continue;
			}

			// Make sure to clear out any existing string data from a previous line read.
			Platform::MemorySet(szKey, 0, 64);
			Platform::MemorySet(szValue, 0, 64);
			
			Platform::MemoryCopy(szKey, &szCurrentLine[1], equalIndex - 1);	// NOTE: Using index 1 here to avoid a dynamic alloc in stripping the string, this just ignored the first '"'.
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';

			// MESHSYSTEMTODO: Kill old code eventually.
			// Process mesh properties.
			if (Utl::StringCompare(szKey, "name"))
			{
				newMeshResource_NEW.pszEntityName = Utl::StringDuplicate(szValue);
			}
			else if (Utl::StringCompare(szKey, "model_resource"))
			{
				// MESHSYSTEMTODO: New System
				// Remove quotes.
				size_t strLen = Utl::StringLength(szValue);
				newMeshResource_NEW.pszModelName = Utl::StringDuplicate_N(&szValue[1], strLen - 2);
				// END NEW SYSTEM

				// Format the path and load up this model resource for use.
				char* pszStrippedNamePointer = Utl::StringStrip(szValue, '\"');
				char szFormattedModelPath[512];

				Utl::StringFormat_V("Assets/Models/%s", szFormattedModelPath, pszStrippedNamePointer);
				Utl::StringFree(pszStrippedNamePointer);
				pszStrippedNamePointer = nullptr;

				if (!MeshLoader::LoadMeshResource(szFormattedModelPath, newMeshResource))
				{
					LOG_ERROR("Failed to load mesh asset '%s'.", szFormattedModelPath);
					return false;
				}
			}
			else if (Utl::StringCompare(szKey, "position"))
			{
				Utl::String_ToVector3(szValue, &newMeshResource.vecPosition);
				Utl::String_ToVector3(szValue, &newMeshResource_NEW.vecPosition);
			}
			else if (Utl::StringCompare(szKey, "rotation"))
			{
				// NOTE: We need to swap the x and y angles because that is how the quaternion operations are expecting them.
				Vector3D vecTempAngles;
				Utl::String_ToVector3(szValue, &vecTempAngles);
				newMeshResource.vecEulerAngles.x = vecTempAngles.y;
				newMeshResource.vecEulerAngles.y = vecTempAngles.x;
				newMeshResource.vecEulerAngles.z = vecTempAngles.z;

				newMeshResource_NEW.vecAngles.x = vecTempAngles.y;
				newMeshResource_NEW.vecAngles.y = vecTempAngles.x;
				newMeshResource_NEW.vecAngles.z = vecTempAngles.z;
			}
			else if (Utl::StringCompare(szKey, "scale"))
			{
				Utl::String_ToVector3(szValue, &newMeshResource.vecScale);
				Utl::String_ToVector3(szValue, &newMeshResource_NEW.vecScale);
			}
			else
			{
				// Handle other misc mesh properties when the time comes.
			}
		}

		// We shouldn't fall out here.
		return false;
	}

	static bool ProcessLightEntity(FileHandle_t& hMapFile, MapResource_t::eLightType type, MapResource_t& pOutMapResource)
	{
		if (FileSystem::IsEOF(&hMapFile))
		{
			// This should NEVER happen!
			// This means that the map file contains a light entry that was not properly terminated.
			LOG_WARN("Map file '%s' contains a formatting error! light entry was not properly termainted!", pOutMapResource.pszMapName);
			return false;
		}

		char	szCurrentLine[kScratchStringBufferLength]{};
		size_t	iCurrentLineLength{ 0 };
		size_t	bytesRead{ 0 };
		size_t	equalIndex{ 0 };
		char	szKey[64]{};
		char	szValue[64]{};

		// Add a new entry to the resource to write to.
		MapResource_t::LightResource_t* pLightResource = nullptr;
		if (type == MapResource_t::eLightType::DIRECTIONAL)
		{
			pLightResource = &pOutMapResource.DirectionalLight;
		}
		else if (type == MapResource_t::eLightType::POINT)
		{
			pOutMapResource.PointLights.PushBack(MapResource_t::LightResource_t{});
			pLightResource = &pOutMapResource.PointLights.Back();
		}
		else if (type == MapResource_t::eLightType::SPOT)
		{
			pLightResource = &pOutMapResource.SpotLight;
		}

		CAssert(pLightResource != nullptr);

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
			if (equalIndex == kInvalidID_U32)
			{
				continue;
			}

			// Make sure to clear out any existing string data from a previous line read.
			Platform::MemorySet(szKey, 0, 64);
			Platform::MemorySet(szValue, 0, 64);

			Platform::MemoryCopy(szKey, &szCurrentLine[1], equalIndex - 1);
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';

			// Process mesh properties.
			if (Utl::StringCompare(szKey, "position"))
			{
				Utl::String_ToVector3(szValue, &pLightResource->vecPosition);
			}
			else if (Utl::StringCompare(szKey, "rotation") || Utl::StringCompare(szKey, "direction"))
			{
				Utl::String_ToVector3(szValue, &pLightResource->vecDirection);
			}
			else if (Utl::StringCompare(szKey, "lightColor"))
			{
				Utl::String_ToVector3(szValue, &pLightResource->lightColor);

				// Bring this value into a range of [0..1]
				pLightResource->lightColor.x /= 255.0f;
				pLightResource->lightColor.y /= 255.0f;
				pLightResource->lightColor.z /= 255.0f;
			}
			else if (Utl::StringCompare(szKey, "ambientLightColor"))
			{
				Utl::String_ToVector3(szValue, &pLightResource->ambientLightColor);

				// Bring this value into a range of [0..1]
				pLightResource->ambientLightColor.x /= 255.0f;
				pLightResource->ambientLightColor.y /= 255.0f;
				pLightResource->ambientLightColor.z /= 255.0f;
			}
			else if (Utl::StringCompare(szKey, "ambientLightStrength"))
			{
				pLightResource->flAmbientStrength = Utl::String_ATOF(&szValue[1]);	// NOTE: Index 1 to get rid of the quotes.
			}
			else if (Utl::StringCompare(szKey, "attenConstant"))
			{
				pLightResource->flConstant = Utl::String_ATOF(&szValue[1]);
			}
			else if (Utl::StringCompare(szKey, "attenLinear"))
			{
				pLightResource->flLinear = Utl::String_ATOF(&szValue[1]);
			}
			else if (Utl::StringCompare(szKey, "attenQuadratic"))
			{
				pLightResource->flQuadratic = Utl::String_ATOF(&szValue[1]);
			}
			else if (Utl::StringCompare(szKey, "brightness"))
			{
				pLightResource->flBrightness = Utl::String_ATOF(&szValue[1]);
			}
			else if (Utl::StringCompare(szKey, "innerAngle"))
			{
				pLightResource->flInnerCutoff = Utl::String_ATOF(&szValue[1]);
			}
			else if (Utl::StringCompare(szKey, "outerAngle"))
			{
				pLightResource->flOuterCutoff = Utl::String_ATOF(&szValue[1]);
			}
			else
			{
				LOG_WARN("Light entry has unused key/value: %s: %s", szKey, szValue);
			}
		}
		
		// We shouldn't fall out here.
		return false;
	}

} // namespace MapLoader
