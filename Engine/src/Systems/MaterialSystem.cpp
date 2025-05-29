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
#include <Resources/Loaders/MaterialLoader.h>
#include <Strings/String_Utils.h>
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"

//-------------------------------------------------------------------------
// Indices for each of the phong shader lookups.
//-------------------------------------------------------------------------
struct PhongShaderLocations_t
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	viewPos{ kInvalidID_U16 };
	uint16_t	dirLightPos{ kInvalidID_U16 };
	uint16_t	renderMode{ kInvalidID_U16 };
	uint16_t	shininess{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	texture_maps{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
};
PhongShaderLocations_t phongShaderLocations{};

//-------------------------------------------------------------------------
// Material System Implementation.
//-------------------------------------------------------------------------
class CMaterialSystem : public IMaterialSystem
{
public:
	virtual bool				Initialize(MaterialSystemConfig_t config) override;
	virtual void				Shutdown() override;

	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) override;

	virtual void				ApplyMaterialInstance(MaterialHandle_t hMaterial) override;

	virtual MaterialHandle_t	AcquireMaterial_OLD(const char* pszMaterialName) override;
	virtual MaterialReference	AcquireMaterialReference(const char* pszMaterialName) override;

	virtual MaterialHandle_t	AcquireMaterialFromConfig(MaterialConfig_t& pConfig) override;
	virtual MaterialReference	AcquireMaterialInternal(const char* pszMaterialName) override;

	virtual MaterialReference	GetDefaultMaterialReference() override;

private:
	void						CreateDefaultMaterial();
	MaterialConfig_t			ParseMaterialConfig(char* pszMaterialPathFile);

	MaterialHandle_t			FindFirstFreeMaterialHandle() const;

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
		MaterialInstanceData_t* m_MaterialInstanceData{ nullptr };
	};

	MaterialDictionary	m_MaterialDictionary{};

	Material			m_DefaultMaterial{};
	MaterialLookup		m_DefaultMaterialLookup{};

	MaterialHandle_t	m_hCurrentlyBoundMaterialInstance{ kInvalidID_U32 };
	ShaderHandle_t		m_DefaultMaterialShader{ kInvalidID_U32 };
};

// Material System context.
static CMaterialSystem s_MaterialSystem;
IMaterialSystem* g_pMaterialSystemInternal = &s_MaterialSystem;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMaterialSystem::Initialize(MaterialSystemConfig_t config)
{
	m_Config = config;
	m_MaterialDictionary.m_MaterialHashmap.Resize(m_Config.MaxMaterials);

	// Allocate memory for our material array.
	m_MaterialDictionary.m_MaterialArray = (Material*)new Material[m_Config.MaxMaterials];
	m_MaterialDictionary.m_MaterialInstanceData = (MaterialInstanceData_t*)new MaterialInstanceData_t[m_Config.MaxMaterials];

	// Default material.
	// TODO: Make this work with new shader system.
	//CreateDefaultMaterial();

	// Setup shader locations.
	m_DefaultMaterialShader = g_pShaderSystem->AcquireShader("Core.Phong");
	phongShaderLocations.projection = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "projection");
	phongShaderLocations.view = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "view");
	phongShaderLocations.viewPos = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "viewPos");
	phongShaderLocations.dirLightPos = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "dirLightPos");
	phongShaderLocations.renderMode = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "renderMode");
	phongShaderLocations.shininess = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "shininess");
	phongShaderLocations.color = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "color");
	phongShaderLocations.texture_maps = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "texture_maps");
	phongShaderLocations.model = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "model");

	LOG_INFO("Material system initialized! (Max materials: %d)", config.MaxMaterials);

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	g_pShaderSystem->ReleaseShader(m_DefaultMaterialShader);

	delete[] m_MaterialDictionary.m_MaterialInstanceData;
	delete[] m_MaterialDictionary.m_MaterialArray;
	LOG_WARN("TODO: IMPLEMENT MATERIAL SYSTEM RESOURCE RELEASE METHOD!!!");
}

//-------------------------------------------------------------------------
// TODO: Documentation
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::AcquireMaterial(const char* pszMaterialName)
{
	MaterialLoader::MaterialResource_t	newMaterialResource{};
	MaterialHandle_t					newMaterialHandle{ kInvalidID_U32 };
	MaterialLookup*						pLookup = &m_MaterialDictionary.m_MaterialHashmap[pszMaterialName];
	Material*							pMaterial = nullptr;

	if (!pszMaterialName)
	{
		LOG_ERROR("AcquireMaterial() called with null material name! Returning default handle.");
		return kInvalidID_U32; // TODO: Default material handle.
	}

	if (pLookup->MaterialIndex == kInvalidID_U32)
	{
		// This material has not yet been loaded, so load it in.

		// Format the path string (TODO: Don't hardcode this path).
		char szFormattedResourcePath[kScratchStringBufferLength];
		StringFormat_V("%s/%s.cmat", szFormattedResourcePath, "Assets/Materials", pszMaterialName);

		// Load the material configuration resource from disk
		if (!MaterialLoader::LoadMaterialResource(szFormattedResourcePath, newMaterialResource))
		{
			LOG_ERROR("Failed to load material resource '%s'. Returning default.", pszMaterialName);
			return 0; // TODO: Default material handle.
		}

		// Find a free slot for this material to occupy.
		newMaterialHandle = FindFirstFreeMaterialHandle();
		if (newMaterialHandle == kInvalidID_U32)
		{
			LOG_ERROR("Failed to create shader '%s'. Returning default handle.", pszMaterialName);
			return kInvalidID_U32;	// TODO: Default material handle.
		}
		pMaterial = &m_MaterialDictionary.m_MaterialArray[newMaterialHandle];

		// Acquire material resources.
		pMaterial->m_szMaterialName = StringDuplicate(newMaterialResource.pszMaterialName);
		pMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader(newMaterialResource.pszShaderName);

		// Acquire shader instance resources for this material.
		g_pShaderSystem->AcquireShaderInstanceResources(pMaterial->m_hShaderHandle, &pMaterial->m_iInternalHandle);

		// Texture maps.
		size_t							textureMapCount = newMaterialResource.textureMapConfigs.Count();
		Utl::CVector<TextureHandle_t>	textureHandles(textureMapCount);

		for (int texIdx = 0; texIdx < textureMapCount; texIdx++)
		{
			TextureHandle_t hTexture = g_pTextureSystem_Internal->AcquireTexture(newMaterialResource.textureMapConfigs[texIdx].pszTextureName, true);
			textureHandles.PushBack(hTexture);
		}

		// Bind the texture maps to the shader instance.
		for (int texIdx = 0; texIdx < textureMapCount; texIdx++)
		{
			// TODO: This only works for the hard-coded phong shader. This will need to be made more robust, on a per-shader basis later.
			int iTextureArrayIndex = (int)newMaterialResource.textureMapConfigs[texIdx].eType;
			g_pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, phongShaderLocations.texture_maps, iTextureArrayIndex, pMaterial->m_iInternalHandle, &textureHandles[texIdx]);
		}
	}

	return newMaterialHandle;
}

//-------------------------------------------------------------------------
// TODO: Documentation
// TODO: Make sure we can assert that a frame is currently being rendered when this function is called.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyMaterialInstance(MaterialHandle_t hMaterial)
{
	CAssert(hMaterial != kInvalidID_U32);

	// Bind the appropriate shader.
	g_pShaderSystem->BindShader(m_MaterialDictionary.m_MaterialArray[hMaterial].m_hShaderHandle);

	// Bind the appropriate material instance.
	g_pShaderSystem->BindPerInstanceUniforms(m_MaterialDictionary.m_MaterialArray[hMaterial].m_iInternalHandle);

	m_hCurrentlyBoundMaterialInstance = hMaterial;
}

//-------------------------------------------------------------------------
// Loads a material at the provided path if it hasn't been loaded.
// Returns a handle to the material.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::AcquireMaterial_OLD(const char* pszMaterialName)
{
	Material* newMaterial = nullptr;
	int matIdx = 0;

	// Make sure our state is ready to go.
	if (!m_MaterialDictionary.m_MaterialArray)
	{
		LOG_WARN("CMaterialSystem::AcquireMaterial_OLD() called before material system was initialized! Returning NULL handle.");
		return 0;
	}

	if (!pszMaterialName)
	{
		LOG_ERROR("CMaterialSystem::AcquireMaterial_OLD() requires a path to the material to load! Returning default material.");
		return m_DefaultMaterialLookup.MaterialIndex;
	}

	// Parse the material file.
	MaterialConfig_t newMatConfig = ParseMaterialConfig(const_cast<char*>(pszMaterialName));

	if (newMatConfig.pszMaterialName == nullptr)
	{
		LOG_ERROR("Failed to load material '%s'. Returning default.", pszMaterialName);
		return kInvalidID_U32;
	}

	// Get an entry into the hashmap.
	MaterialLookup* newMaterialLookup = &m_MaterialDictionary.m_MaterialHashmap[pszMaterialName];
	if (!newMaterialLookup)
	{
		LOG_ERROR("Failed to find free space for a new material in the material hashmap, returning default.");
		return m_DefaultMaterialLookup.MaterialIndex; 
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
			return m_DefaultMaterialLookup.MaterialIndex;
		}
		MaterialInstanceData_t& pMatInstanceData = m_MaterialDictionary.m_MaterialInstanceData[matIdx];

		// Acquire material resources.
		// TODO: we'll need to acquire a shader handle here eventually...
		if (newMatConfig.pszMaterialName != nullptr)
		{
			newMaterial->m_szMaterialName = StringDuplicate(newMatConfig.pszMaterialName);
		}

		if (newMatConfig.pszShaderName != nullptr)
		{
			newMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader(newMatConfig.pszMaterialName);
		}

		if (newMatConfig.iShininess != kInvalidID_U32)
		{
			newMaterial->m_iShininess = newMatConfig.iShininess;
		}
		else
		{
			newMaterial->m_iShininess = 8; // A decent default.
		}
		pMatInstanceData.Shininess = newMaterial->m_iShininess;
		
		if (newMatConfig.pszDiffuseTextureName != nullptr)
		{
			//newMaterial->m_hDiffuseTextureHandle = g_pTextureSystem->AcquireTexture_OLD(newMatConfig.pszDiffuseTextureName, eKnownTextureTypes::TEXTURE_TYPE_DIFFUSE);
			newMaterial->m_hDiffuseTextureHandle = g_pTextureSystem_Internal->AcquireTexture(newMatConfig.pszDiffuseTextureName, true);

			// If the diffuse texture is transparent, mark the material as transparent.
			if ((g_pTextureSystem_Internal->GetTextureFromHandle(newMaterial->m_hDiffuseTextureHandle)->GetFlags() & TEXTURE_FLAG_IS_TRANSPARENT) != 0)
				newMaterial->m_bIsTransparent = true;
		}

		if (newMatConfig.pszSpecularTextureName != nullptr)
		{
			//newMaterial->m_hSpecularTextureHandle = g_pTextureSystem->AcquireTexture_OLD(newMatConfig.pszSpecularTextureName, eKnownTextureTypes::TEXTURE_TYPE_SPECULAR);
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem_Internal->AcquireTexture(newMatConfig.pszSpecularTextureName, true);
		}
		else
		{
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem_Internal->GetDefaultSpecularTexture();
			LOG_VERBOSE("Material '%s' missing specular texture, using default.", pszMaterialName);
		}

		if (newMatConfig.pszNormalTextureName != nullptr)
		{
			//newMaterial->m_hNormalTextureHandle = g_pTextureSystem->AcquireTexture_OLD(newMatConfig.pszNormalTextureName, eKnownTextureTypes::TEXTURE_TYPE_NORMAL);
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem_Internal->AcquireTexture(newMatConfig.pszNormalTextureName, true);
		}
		else
		{
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem_Internal->GetDefaultNormalTexture();
			LOG_VERBOSE("Material '%s' missing normal texture, using default.", pszMaterialName);
		}

		// Acquire shader resources.
		if (!g_pRenderer->AcquireMaterialResources_OLD(*newMaterial, m_MaterialDictionary.m_MaterialInstanceData[matIdx]))
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
	MaterialHandle_t newMaterial = AcquireMaterial_OLD(pszMaterialName);

	MaterialReference newRef;

	if (newMaterial == kInvalidID_U32)
	{
		// Return default material.
		return GetDefaultMaterialReference();
	}

	newRef.m_iInternalHandle = m_MaterialDictionary.m_MaterialArray[newMaterial].m_iInternalHandle;
	newRef.m_hShaderHandle = m_MaterialDictionary.m_MaterialArray[newMaterial].m_hShaderHandle;
	newRef.m_hDiffuseTextureHandle = m_MaterialDictionary.m_MaterialArray[newMaterial].m_hDiffuseTextureHandle;

	return newRef;
}

MaterialHandle_t CMaterialSystem::AcquireMaterialFromConfig(MaterialConfig_t& pConfig)
{
	Material* newMaterial = nullptr;
	int matIdx = 0;

	// Make sure our state is ready to go.
	if (!m_MaterialDictionary.m_MaterialArray)
	{
		LOG_WARN("CMaterialSystem::AcquireMaterial_OLD() called before material system was initialized! Returning NULL handle.");
		return 0;
	}

	if (pConfig.pszMaterialName == nullptr)
	{
		LOG_ERROR("Failed to load material '%s'. Returning default.", pConfig.pszMaterialName);
		return m_DefaultMaterialLookup.MaterialIndex;
	}

	// Get an entry into the hashmap.
	MaterialLookup* newMaterialLookup = &m_MaterialDictionary.m_MaterialHashmap[pConfig.pszMaterialName];
	if (!newMaterialLookup)
	{
		LOG_ERROR("Failed to find free space for a new material in the material hashmap, returning default.");
		return m_DefaultMaterialLookup.MaterialIndex;
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
			LOG_ERROR("Could not load material '%s' because all material slots are full, returning default material.", pConfig.pszMaterialName);
			return m_DefaultMaterialLookup.MaterialIndex;
		}
		MaterialInstanceData_t& pMatInstanceData = m_MaterialDictionary.m_MaterialInstanceData[matIdx];

		// Acquire material resources.
		if (pConfig.pszMaterialName != nullptr)
		{
			newMaterial->m_szMaterialName = StringDuplicate(pConfig.pszMaterialName);
		}

		if (pConfig.pszShaderName != nullptr)
		{
			newMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader(pConfig.pszShaderName);
		}

		if (pConfig.iShininess != kInvalidID_U32)
		{
			newMaterial->m_iShininess = pConfig.iShininess;
		}
		else
		{
			newMaterial->m_iShininess = 8; // A decent default.
		}
		pMatInstanceData.Shininess = newMaterial->m_iShininess;

		if (pConfig.pszDiffuseTextureName != nullptr)
		{
			newMaterial->m_hDiffuseTextureHandle = g_pTextureSystem_Internal->AcquireTexture(pConfig.pszDiffuseTextureName, true);

			// If the diffuse texture is transparent, mark the material as transparent.
			if ((g_pTextureSystem_Internal->GetTextureFromHandle(newMaterial->m_hDiffuseTextureHandle)->GetFlags() & TEXTURE_FLAG_IS_TRANSPARENT) != 0)
				newMaterial->m_bIsTransparent = true;
		}

		if (pConfig.pszSpecularTextureName != nullptr)
		{
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem_Internal->AcquireTexture(pConfig.pszSpecularTextureName, true);
		}
		else
		{
			newMaterial->m_hSpecularTextureHandle = g_pTextureSystem_Internal->GetDefaultSpecularTexture();
			LOG_VERBOSE("Material '%s' missing specular texture, using default.", pConfig.pszMaterialName);
		}

		if (pConfig.pszNormalTextureName != nullptr)
		{
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem_Internal->AcquireTexture(pConfig.pszNormalTextureName, true);
		}
		else
		{
			newMaterial->m_hNormalTextureHandle = g_pTextureSystem_Internal->GetDefaultNormalTexture();
			LOG_VERBOSE("Material '%s' missing normal texture, using default.", pConfig.pszMaterialName);
		}
		pMatInstanceData.vecDiffuseColor = pConfig.vecDiffuseColor;
		// Acquire shader resources.
		if (!g_pRenderer->AcquireMaterialResources_OLD(*newMaterial, m_MaterialDictionary.m_MaterialInstanceData[matIdx]))
		{
			LOG_ERROR("Failed to acquire renderer resources for material '%s'. Returning default material.", pConfig.pszMaterialName);
			return m_DefaultMaterialLookup.MaterialIndex; // TODO: Release resources.
		}
	}
	else
	{
		newMaterial = &m_MaterialDictionary.m_MaterialArray[newMaterialLookup->MaterialIndex];
	}

	if (!newMaterial)
	{
		LOG_ERROR("Failed to load/create material '%s'. Returning default material.", pConfig.pszMaterialName);
		return m_DefaultMaterialLookup.MaterialIndex;
	}

	if (newMaterialLookup->MaterialIndex == kInvalidID_U32)
		newMaterialLookup->MaterialIndex = matIdx;

	newMaterialLookup->ReferenceCount++;
	newMaterialLookup->AutoRelease = true; // TODO: Configurable.

	LOG_VERBOSE("Successfully acquired material: '%s', at index %d, ref. count: %d", newMaterial->m_szMaterialName, newMaterialLookup->MaterialIndex, newMaterialLookup->ReferenceCount);

	return newMaterialLookup->MaterialIndex;
}

MaterialReference CMaterialSystem::AcquireMaterialInternal(const char* pszMaterialName)
{
	MaterialLookup& pLookup = m_MaterialDictionary.m_MaterialHashmap[pszMaterialName];
	MaterialReference newRef{};
	if (pLookup.MaterialIndex != kInvalidID_U32 && m_MaterialDictionary.m_MaterialArray[pLookup.MaterialIndex].m_iInternalHandle != kInvalidID_U32)
	{
		newRef.m_iInternalHandle = m_MaterialDictionary.m_MaterialArray[pLookup.MaterialIndex].m_iInternalHandle;
		newRef.m_hShaderHandle = m_MaterialDictionary.m_MaterialArray[pLookup.MaterialIndex].m_hShaderHandle;
		newRef.m_hDiffuseTextureHandle = m_MaterialDictionary.m_MaterialArray[pLookup.MaterialIndex].m_hDiffuseTextureHandle;
		newRef.m_bIsTransparent = m_MaterialDictionary.m_MaterialArray[pLookup.MaterialIndex].m_bIsTransparent;
		return newRef;
	}
	else
	{
		LOG_ERROR("Failed to find internal reference for material '%s'.", pszMaterialName);
		return GetDefaultMaterialReference();
	}
}

//-------------------------------------------------------------------------
// Returns a reference to the default texture.
//-------------------------------------------------------------------------
MaterialReference CMaterialSystem::GetDefaultMaterialReference()
{
	MaterialReference defaultMaterialRef{};

	defaultMaterialRef.m_iInternalHandle = m_DefaultMaterial.m_iInternalHandle;
	defaultMaterialRef.m_hShaderHandle = m_DefaultMaterial.m_hShaderHandle;
	defaultMaterialRef.m_hDiffuseTextureHandle = m_DefaultMaterial.m_hDiffuseTextureHandle;

	return defaultMaterialRef;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
void CMaterialSystem::CreateDefaultMaterial()
{
	m_DefaultMaterial.m_szMaterialName = StringDuplicate("_DefaultMaterial");
	//m_DefaultMaterial.m_hShaderHandle = g_pShaderSystem->AcquireShader_OLD("Core.Phong");	// TODO: NEW SHADER METHOD
	m_DefaultMaterial.m_hDiffuseTextureHandle = g_pTextureSystem_Internal->GetDefaultDiffuseTexture();
	m_DefaultMaterial.m_hSpecularTextureHandle = g_pTextureSystem_Internal->GetDefaultSpecularTexture();
	m_DefaultMaterial.m_hNormalTextureHandle = g_pTextureSystem_Internal->GetDefaultNormalTexture();
	MaterialInstanceData_t defaultInstanceData{};
	defaultInstanceData.Shininess = 32;

	int matIdx = 0;
	Material* pNewMaterial = nullptr;
	for (matIdx = 0; matIdx < m_Config.MaxMaterials; matIdx)
	{
		if (m_MaterialDictionary.m_MaterialArray[matIdx].m_iInternalHandle == kInvalidID_U32)
		{
			pNewMaterial = &m_MaterialDictionary.m_MaterialArray[matIdx];
			break;
		}
	}

	MaterialLookup* newMaterialLookup = &m_MaterialDictionary.m_MaterialHashmap[m_DefaultMaterial.m_szMaterialName];
	CAssert(newMaterialLookup->MaterialIndex == kInvalidID_U32); // Make sure this slot is not in use.
	newMaterialLookup->MaterialIndex = matIdx;

	g_pRenderer->AcquireMaterialResources_OLD(m_DefaultMaterial, defaultInstanceData);
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
		else if (StringCompare(key, "shininess"))
		{
			newMatConfig.iShininess = String_ATOI(value);
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

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::FindFirstFreeMaterialHandle() const
{
	for (int idx = 0; idx < m_Config.MaxMaterials; idx++)
	{
		if (m_MaterialDictionary.m_MaterialArray[idx].m_iInternalHandle == kInvalidID_U32)
			return idx;
	}

	LOG_ERROR("Failed to find a free slot in the material dictionary. Adjust config to allow for more.");
	return kInvalidID_U32;
}