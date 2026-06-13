//=============================================================================
// Author: Jared Simons
// Date Created: 5/28/2025
// 
// Purpose: Material resource loading utilities.
//=============================================================================
#include "MaterialLoader.h"

#include "Core/Assertions.h"
#include "Core/Logging.h"
#include "Platform/FileSystem.h"

namespace MaterialLoader
{
	// @Multithreading: This function is a prime candidate for threading.
	//-------------------------------------------------------------------------
	// Purpose: Attempts to load a material asset from disk.
	// 
	// Input:	pszMaterialPath - The path to the material to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadMaterialResource(const char* pszMaterialPath, MaterialResource_t& pOutResource)
	{
		// Make sure the file actually exists first.
		if (!FileSystem::FileExists(pszMaterialPath))
		{
			LOG_ERROR("Material config file '%s' could not be found. Material resource load failed.", pszMaterialPath);
			return false;
		}

		FileHandle_t hMaterialConfigFile{};
		if (!FileSystem::OpenFile(pszMaterialPath, eFileMode::FILEMODE_READ, false, &hMaterialConfigFile))
		{
			return false;
		}

		// The name of the material will be its name on disk without the path and file extension.
		pOutResource.pszMaterialName = Utl::StringGetFilenameFromPath(pszMaterialPath);
		pOutResource.textureMapConfigs.Resize(5);

		// Parse the config and write it to the resource.
		while (!FileSystem::IsEOF(&hMaterialConfigFile))
		{
			char	szCurrentLine[kScratchStringBufferLength];
			u64	currentLineLength{ 0 };
			u64	bytesRead{ 0 };
			u64	equalIndex{ 0 };
			char	szKey[500];
			char	szValue[500];
			
			FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &hMaterialConfigFile);
			currentLineLength = Utl::StringLength(szCurrentLine);

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			equalIndex = Utl::StringIndexOf(szCurrentLine, '=');
			CAssert(equalIndex != kInvalidID_U32);
			Platform::MemoryCopy(szKey, szCurrentLine, equalIndex);
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';
	
			if (Utl::StringCompare(szKey, "shader"))
			{
				pOutResource.pszShaderName = Utl::StringDuplicate(szValue);
			}
			else if (Utl::StringCompare(szKey, "map_diffuse"))
			{
				// Diffuse texture map.
				MaterialResource_t::textureMapConfig_t diffuseMap{};
				diffuseMap.pszTextureName = Utl::StringDuplicate(szValue);
				diffuseMap.eType = MaterialResource_t::eTextureType::DIFFUSE;
				pOutResource.textureMapConfigs.PushBack(diffuseMap);
			}
			else if (Utl::StringCompare(szKey, "map_specular"))
			{
				// Specular texture map.
				MaterialResource_t::textureMapConfig_t specularMap{};
				specularMap.pszTextureName = Utl::StringDuplicate(szValue);
				specularMap.eType = MaterialResource_t::eTextureType::SPECULAR;
				pOutResource.textureMapConfigs.PushBack(specularMap);
			}
			else if (Utl::StringCompare(szKey, "map_normal"))
			{
				// Normal texture map.
				MaterialResource_t::textureMapConfig_t normalMap{};
				normalMap.pszTextureName = Utl::StringDuplicate(szValue);
				normalMap.eType = MaterialResource_t::eTextureType::NORMAL;
				pOutResource.textureMapConfigs.PushBack(normalMap);
			}
			else
			{
				// Must be a custom keyvalue.
				pOutResource.pszCustomKeys.PushBack(Utl::StringDuplicate(szKey));
				pOutResource.pszCustomValues.PushBack(Utl::StringDuplicate(szValue));
			}
		}

		FileSystem::CloseFile(&hMaterialConfigFile);

		return true;
	}
}