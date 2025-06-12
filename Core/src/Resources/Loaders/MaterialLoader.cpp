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
	// TODO: Documentation
	//-------------------------------------------------------------------------
	bool LoadMaterialResource(const char* pszMaterialPath, struct MaterialResource_t& pOutResource)
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

		// Parse the config and write it to the resource.
		while (!FileSystem::IsEOF(&hMaterialConfigFile))
		{
			char	szCurrentLine[kScratchStringBufferLength];
			size_t	currentLineLength{ 0 };
			size_t	bytesRead{ 0 };
			size_t	equalIndex{ 0 };
			char	szKey[500];
			char	szValue[500];
			
			FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &hMaterialConfigFile);
			currentLineLength = StringLength(szCurrentLine);

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			equalIndex = StringIndexOf(szCurrentLine, '=');
			CAssert(equalIndex != kInvalidID_U32);
			Platform::MemoryCopy(szKey, szCurrentLine, equalIndex);
			szKey[equalIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
			szValue[bytesRead - 1] = '\0';

			if (StringCompare(szKey, "name"))
			{
				pOutResource.pszMaterialName = StringDuplicate(szValue);
			}
			else if (StringCompare(szKey, "shader"))
			{
				pOutResource.pszShaderName = StringDuplicate(szValue);
			}
			else if (StringCompare(szKey, "map_diffuse"))
			{
				// Diffuse texture map.
				MaterialResource_t::eTextureMapConfig_t diffuseMap{};
				diffuseMap.pszTextureName = StringDuplicate(szValue);
				diffuseMap.eType = MaterialResource_t::eTextureType::DIFFUSE;
				pOutResource.textureMapConfigs.PushBack(diffuseMap);
			}
			else if (StringCompare(szKey, "map_specular"))
			{
				// Specular texture map.
				MaterialResource_t::eTextureMapConfig_t specularMap{};
				specularMap.pszTextureName = StringDuplicate(szValue);
				specularMap.eType = MaterialResource_t::eTextureType::SPECULAR;
				pOutResource.textureMapConfigs.PushBack(specularMap);
			}
			else if (StringCompare(szKey, "map_normal"))
			{
				// Specular texture map.
				MaterialResource_t::eTextureMapConfig_t normalMap{};
				normalMap.pszTextureName = StringDuplicate(szValue);
				normalMap.eType = MaterialResource_t::eTextureType::NORMAL;
				pOutResource.textureMapConfigs.PushBack(normalMap);
			}
			else
			{
				LOG_WARN("Failed to parse material key: '%s'.", szKey);
			}

			int myBreakPoint = 0;
		}

		FileSystem::CloseFile(&hMaterialConfigFile);

		return true;
	}
}