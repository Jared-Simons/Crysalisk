//=============================================================================
// Author: Jared Simons
// Date Created: 3/3/2025
// 
// Purpose: The material system.
//=============================================================================

#include "MaterialSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>
#include "Systems/TextureSystem.h"

//-------------------------------------------------------------------------
// Material System Implementation.
//-------------------------------------------------------------------------
class CMaterialSystem : public IMaterialSystem
{
public:
	virtual bool	Initialize(MaterialSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) override;
	virtual MaterialReference	AcquireMaterialReference(const char* pszMaterialName) override;

private:
	MaterialConfig_t			ParseMaterialConfig(char* pszMaterialPathFile);

private:
	MaterialSystemConfig_t	m_Config{};

	struct MaterialLookup
	{
		MaterialHandle_t MaterialIndex{ kInvalidID_U32 };
		int				 ReferenceCount{ 0 };
		bool			 AutoRelease{ false };
	};

	struct MaterialDictionary
	{
		Utl::CHashMap<const char*, MaterialLookup> m_MaterialHashmap{};
		Material* m_MaterialArray{ nullptr };
	};

	MaterialDictionary	m_MaterialDictionary{};
};

// Material System context.
static CMaterialSystem s_MaterialSystem;
IMaterialSystem* g_pMaterialSystem = &s_MaterialSystem;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMaterialSystem::Initialize(MaterialSystemConfig_t config)
{
	m_Config = config;
	m_MaterialDictionary.m_MaterialHashmap.Resize(m_Config.MaxMaterials);

	// Allocate memory for our material array.
	m_MaterialDictionary.m_MaterialArray = (Material*)new Material[m_Config.MaxMaterials];

	LOG_INFO("Material system initialized! (Max materials: %d)", config.MaxMaterials);

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	delete[] m_MaterialDictionary.m_MaterialArray;
	LOG_WARN("TODO: IMPLEMENT MATERIAL SYSTEM RESOURCE RELEASE METHOD!!!");
}

//-------------------------------------------------------------------------
// Loads a material at the provided path if it hasn't been loaded.
// Returns a handle to the material.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::AcquireMaterial(const char* pszMaterialName)
{
	Material* newMaterial = nullptr;
	int matIdx = 0;

	// Make sure our state is ready to go.
	if (!m_MaterialDictionary.m_MaterialArray)
	{
		LOG_WARN("CMaterialSystem::AcquireMaterial() called before material system was initialized! Returning NULL handle.");
		return 0;
	}

	if (!pszMaterialName)
	{
		LOG_ERROR("CMaterialSystem::AcquireMaterial() requires a path to the material to load! Returning default material.");
		return 0; // TODO: Default material.
	}

	// Get an entry into the hashmap.
	MaterialLookup* newMaterialLookup = m_MaterialDictionary.m_MaterialHashmap.Insert(pszMaterialName);
	if (!newMaterialLookup)
	{
		LOG_ERROR("Failed to find free space for a new material in the material hashmap, returning default.");
		return 0; // TODO: Default material.
	}

	if (newMaterialLookup->MaterialIndex == kInvalidID_U32) // This material does not exist in the hashmap yet.
	{
		// Get at the first free material.
		for (matIdx = 0; matIdx < m_Config.MaxMaterials; matIdx++)
		{
			// Our heuristic for finding a free texture right now is that the material name is null and the diffuse texture is invalid.
			if (!m_MaterialDictionary.m_MaterialArray[matIdx].m_szMaterialName && m_MaterialDictionary.m_MaterialArray[matIdx].m_hDiffuseTextureHandle == kInvalidID_U32)
			{
				newMaterial = &m_MaterialDictionary.m_MaterialArray[matIdx];
				break;
			}
		}

		if (!newMaterial)
		{
			LOG_ERROR("Could not load material '%s' because all material slots were full, returning default material.", pszMaterialName);
			return 0; // TODO: Default material.
		}

		// Parse the material file.
		MaterialConfig_t newMatConfig = ParseMaterialConfig(const_cast<char*>(pszMaterialName));

		// Acquire material resources.
		// TODO: we'll need to acquire a shader handle here eventually...
		if (newMatConfig.pszMaterialName != nullptr)
		{
			newMaterial->m_szMaterialName = StringDuplicate(newMatConfig.pszMaterialName);
		}
		
		if (newMatConfig.pszDiffuseTextureName != nullptr)
		{
			newMaterial->m_hDiffuseTextureHandle = g_pTextureSystem->AcquireTexture(newMatConfig.pszDiffuseTextureName);
		}

		if (newMatConfig.pszSpecularTextureName != nullptr)
		{
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem->AcquireTexture(newMatConfig.pszSpecularTextureName);
		}
		else
		{
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem->GetDefaultSpecularTexture();
			LOG_VERBOSE("Material '%s' missing specular texture, using default.", pszMaterialName);
		}

		if (newMatConfig.pszNormalTextureName != nullptr)
		{
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem->AcquireTexture(newMatConfig.pszNormalTextureName);
		}
		else
		{
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem->GetDefaultNormalTexture();
			LOG_VERBOSE("Material '%s' missing normal texture, using default.", pszMaterialName);
		}

		// Acquire shader resources.
		if (!g_pRenderer->AcquireMaterialResources(*newMaterial))
		{
			LOG_ERROR("Failed to acquire renderer resources for material '%s'. Returning default material.", pszMaterialName);
			return 0; // TODO: Default texture and release resources.
		}
	}
	else
	{
		newMaterial = &m_MaterialDictionary.m_MaterialArray[newMaterialLookup->MaterialIndex];
	}

	if (!newMaterial)
	{
		LOG_ERROR("Failed to load/create material '%s'. Returning default material.", pszMaterialName);
		return 0; // TODO: Default material.
	}

	if (newMaterialLookup->MaterialIndex == kInvalidID_U32)
		newMaterialLookup->MaterialIndex = matIdx;

	newMaterialLookup->ReferenceCount++;
	newMaterialLookup->AutoRelease = true; // TODO: Configurable.

	LOG_VERBOSE("Successfully acquired material: '%s', at index %d, ref. count: %d", newMaterial->m_szMaterialName, newMaterialLookup->MaterialIndex, newMaterialLookup->ReferenceCount);

	return newMaterialLookup->MaterialIndex;
}

//-------------------------------------------------------------------------
// Loads a material at the provided path if it hasn't been loaded.
// Returns a reference to the material.
//-------------------------------------------------------------------------
MaterialReference CMaterialSystem::AcquireMaterialReference(const char* pszMaterialName)
{
	MaterialHandle_t newMaterial = AcquireMaterial(pszMaterialName);

	MaterialReference newRef;
	newRef.m_iInternalHandle = m_MaterialDictionary.m_MaterialArray[newMaterial].m_iInternalHandle;
	newRef.m_hDiffuseTextureHandle = m_MaterialDictionary.m_MaterialArray[newMaterial].m_hDiffuseTextureHandle;
	return newRef;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
MaterialConfig_t CMaterialSystem::ParseMaterialConfig(char* pszMaterialFilePath)
{
	MaterialConfig_t newMatConfig{};

	// Open the material file.
	// (after appending the extension to the path).
	char* formattedPath = StringConcat(pszMaterialFilePath, ".cmat");
	FileHandle_t matConfigFile{};
	if (!FileSystem::OpenFile(formattedPath, eFileMode::FILEMODE_READ, false, &matConfigFile))
	{
		LOG_ERROR("ParseMaterialConfig -> Could not find material file '%s'. Nothing was done.", pszMaterialFilePath);
		StringFree(formattedPath);
		return newMatConfig;
	}

	while (!FileSystem::IsEOF(&matConfigFile))
	{
		char currentLine[14000]; // Would be a suprise if this is not enough...
		size_t currentLineLength{ 0 };
		size_t bytesRead{ 0 };
		size_t equalIndex{ 0 };
		char key[500];
		char value[500];
		FileSystem::ReadLine(currentLine, 14000, &bytesRead, &matConfigFile);
		currentLineLength = StringLength(currentLine);

		if (currentLineLength <= 0 || currentLine[0] == '#') // Blank line or comment
			continue;

		equalIndex = StringIndexOf(currentLine, '=');
		memcpy(key, currentLine, equalIndex);
		key[equalIndex] = '\0';
		memcpy(value, &currentLine[equalIndex + 1], bytesRead - equalIndex);
		value[bytesRead - 1] = '\0';

		if (StringCompare(key, "name"))
		{
			newMatConfig.pszMaterialName = StringDuplicate(value);
		}
		else if (StringCompare(key, "shader"))
		{
			newMatConfig.pszShaderName = StringDuplicate(value);
		}
		else if (StringCompare(key, "map_diffuse"))
		{
			newMatConfig.pszDiffuseTextureName = StringDuplicate(value);
		}
		else if (StringCompare(key, "map_specular"))
		{
			newMatConfig.pszSpecularTextureName = StringDuplicate(value);
		}
		else if (StringCompare(key, "map_normal"))
		{
			newMatConfig.pszNormalTextureName = StringDuplicate(value);
		}
		else
		{
			LOG_WARN("Failed to parse material key: '%s'.", key);
		}
	}

	StringFree(formattedPath);
	FileSystem::CloseFile(&matConfigFile);

	return newMatConfig;
}
