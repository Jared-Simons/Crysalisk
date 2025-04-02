//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The texture manager.
//=============================================================================

#include "TextureSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Memory/MemoryManager.h>
#include "Renderer/Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <Vendor/stb_image.h>

//-------------------------------------------------------------------------
// Texture System Implementation.
//-------------------------------------------------------------------------
class CTextureSystem : public ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName) override;
	virtual void				ReleaseTexture(TextureHandle_t hTexture, bool bForceRelease) override;

			// This variant exists to free textures in the array without having to access the hashmap from the calling function. This really only exists to easily clear the textures on shutdown.
			void				ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease);

	// Use this to convert a texture handle to a texture pointer. This is necessary so the renderer can get at texture resources without relying on the texture system.
	// Nothing except the renderer should be using this, handles are the right answer elsewhere.
	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) override { return &m_TextureDictionary.m_TextureArray[hTex]; }

	virtual TextureHandle_t		GetDefaultDiffuseTexture() override { return m_pDefaultTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultWhiteTexture() override { return m_pWhiteTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultSpecularTexture() override { return m_pDefaultSpecularTexture->TextureIndex; }
	virtual TextureHandle_t		GetDefaultNormalTexture() override { return m_pDefaultNormalTexture->TextureIndex; }

	bool			CreateDefaultTextures();
	void			DestroyDefaultTextures();

private:
	TextureSystemConfig_t	m_Config{};

	struct TextureLookup
	{
		TextureHandle_t	TextureIndex{ kInvalidID_U32 };
		int				ReferenceCount{ 0 };
		bool			AutoRelease{ false };
	};

	struct TextureDictionary
	{
		Utl::CHashMap<const char*, TextureLookup> m_TextureHashmap{};
		Texture* m_TextureArray{ nullptr };
	};

	TextureDictionary	m_TextureDictionary{};

	// Default textures:
	TextureLookup* m_pDefaultTextureLookup{ nullptr };
	TextureLookup* m_pWhiteTextureLookup{ nullptr };
	TextureLookup* m_pDefaultSpecularTexture{ nullptr };
	TextureLookup* m_pDefaultNormalTexture{ nullptr };
};

// Texture System context.
static CTextureSystem s_TextureSystem;
ITextureSystem* g_pTextureSystem = &s_TextureSystem;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CTextureSystem::Initialize(TextureSystemConfig_t config)
{
	m_Config = config;
	m_TextureDictionary.m_TextureHashmap.Resize(m_Config.MaxTextures);

	// Allocate memory for our texture array.
	m_TextureDictionary.m_TextureArray = (Texture*)new Texture[m_Config.MaxTextures];

	if (!CreateDefaultTextures())
	{
		LOG_ERROR("Failed to create default textures!");
		return false;
	}

	LOG_INFO("Texture system initialized! (Max textures: %d)", config.MaxTextures);

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CTextureSystem::Shutdown()
{
	// Free any texture resources in use by the renderer.
	for (int idx = 0; idx < m_Config.MaxTextures; idx++)
	{
		if (m_TextureDictionary.m_TextureArray[idx].m_pInternalData != nullptr)
		{
			ReleaseTextureAtIndex(m_TextureDictionary.m_TextureArray[idx], true); // True to force the texture system to release resources.
		}
	}

	// Release default textures.
	DestroyDefaultTextures();

	delete[] m_TextureDictionary.m_TextureArray;
}

//-------------------------------------------------------------------------
// Loads a texture at the provided path if it hasn't been loaded.
// Returns a handle to the texture.
//-------------------------------------------------------------------------
TextureHandle_t CTextureSystem::AcquireTexture(const char* pszTextureName)
{
	Texture* newTexture = nullptr;
	int texIdx = 0;

	// Make sure our state is ready to go.
	if (!m_TextureDictionary.m_TextureArray)
	{
		LOG_WARN("CTextureSystem::AcquireTexture() called before texture system was initialized! Returning NULL handle.");
		return 0; // TODO: Invalid ID.
	}

	if (!pszTextureName)
	{
		LOG_ERROR("CTextureSystem::AcquireTexture() requires a path to the texture to load! Returning default texture.");
		return m_pDefaultTextureLookup->TextureIndex;
	}

	// We need to flip standard images.
	stbi_set_flip_vertically_on_load(true);

	// Get an entry into the hashmap.
	TextureLookup* newTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(pszTextureName);

	if (!newTextureLookup)
	{
		LOG_ERROR("Failed to find free space for new texture in texture hashmap, returning default.");
		return m_pDefaultTextureLookup->TextureIndex;
	}

	if (newTextureLookup->TextureIndex == kInvalidID_U32) // This texture does not exist in the hashmap yet.
	{
		// Get at the first free texture.
		for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
		{
			if (!m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
			{
				// Found one!
				newTexture = &m_TextureDictionary.m_TextureArray[texIdx];
				break;
			}
		}

		if (!newTexture)
		{
			LOG_ERROR("Could not load texture '%s' because all texture slots were full, returning default texture.", pszTextureName);
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Load up the image
		byte* pPixelData = stbi_load(pszTextureName, &newTexture->width, &newTexture->height, &newTexture->numChannels, 4);

		if (!pPixelData)
		{
			LOG_ERROR("Failed to load texture '%s'. Returning default texture.", pszTextureName); // TODO: Failure reason?
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Send the data off to the renderer to be uploaded to the GPU.
		if (!g_pRenderer->AcquireTextureResources(*newTexture, texIdx, (newTexture->width * newTexture->height * newTexture->numChannels), pPixelData))
		{
			LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Returning default texture.", pszTextureName);
			stbi_image_free(pPixelData);
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Free the original pixel data.
		stbi_image_free(pPixelData);

		newTexture->pszTextureName = StringDuplicate(pszTextureName);
	}
	else
	{
		newTexture = &m_TextureDictionary.m_TextureArray[newTextureLookup->TextureIndex];
	}

	if (!newTexture)
	{
		LOG_ERROR("Failed to find resources to load/create texture '%s'. Returning default texture.", pszTextureName);
		return m_pDefaultTextureLookup->TextureIndex;
	}

	if (newTextureLookup->TextureIndex == kInvalidID_U32)
		newTextureLookup->TextureIndex = texIdx;
	
	newTextureLookup->ReferenceCount++;
	newTextureLookup->AutoRelease = true; // TODO: Configurable.
	
	LOG_VERBOSE("Successfully acquired texture: '%s', at index: %d, ref. count: %d", newTexture->pszTextureName, newTextureLookup->TextureIndex, newTextureLookup->ReferenceCount);
	return newTextureLookup->TextureIndex;
}

//-------------------------------------------------------------------------
// Releases the texture, which decrements it's reference count.
// Once that count reaches zero, textures that are flagged as such will be released.
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTexture(TextureHandle_t hTexture, bool bForceRelease)
{
	Texture* pTexture = &m_TextureDictionary.m_TextureArray[hTexture];

	// Default textures do not go through this pipe.
	if (pTexture->textureFlags & TEXTURE_FLAG_DEFAULT_TEXTURE)
		return;

	// Get the texture lookup.
	TextureLookup* pTexLookup = &m_TextureDictionary.m_TextureHashmap[pTexture->pszTextureName];
	if (!pTexLookup || pTexLookup->TextureIndex == kInvalidID_U32)
	{
		LOG_ERROR("Texture '%s' failed to release resources. Texture dictionary is likely corrupt!", pTexture->pszTextureName);
		return;
	}

	// Decrement reference count.
	pTexLookup->ReferenceCount--;
	LOG_VERBOSE("Released texture '%s', reference count is now %d", pTexture->pszTextureName, pTexLookup->ReferenceCount);

	// Release resources if necessary.
	if ((pTexLookup->ReferenceCount <= 0 && pTexLookup->AutoRelease == true) || bForceRelease)
	{
		if (bForceRelease)
		{
			LOG_DEBUG("Texture '%s''s reference count has reached %d, but is being force-released. Releasing resources.", pTexture->pszTextureName, pTexLookup->ReferenceCount);
		}
		else
		{
			LOG_DEBUG("Texture '%s''s reference count has reached %d, and is set to auto-release. Releasing resources.", pTexture->pszTextureName, pTexLookup->ReferenceCount);
		}
		g_pRenderer->ReleaseTextureResources(*pTexture);
		if (pTexture->pszTextureName)
			StringFree(pTexture->pszTextureName);
	}
}

//-------------------------------------------------------------------------
// Same as above, except this goes right to the texture array, skipping the lookup in the hashmap.
// This is used so that we don't need to get the table lookup on shutdown, when we just want to release all texture resources anyways.
// NOTE THAT THIS WILL FREE RESOURCES WITH NO REGARD TO REF. COUNTING!!!
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease)
{
	// Default textures do not go through this pipe.
	if (pTexture.textureFlags & TEXTURE_FLAG_DEFAULT_TEXTURE)
		return;

	g_pRenderer->ReleaseTextureResources(pTexture);
	if (pTexture.pszTextureName)
		StringFree(pTexture.pszTextureName);

	pTexture.m_pInternalData = nullptr;
}

//-------------------------------------------------------------------------
// Create/Acquire default texture resources.
//-------------------------------------------------------------------------
bool CTextureSystem::CreateDefaultTextures()
{
	LOG_INFO("Creating default textures");

	// Default diffuse texture
	{
		Texture* defaultDiffuseTexture = nullptr;
		int texIdx = 0;

		// Get an entry into the hashmap.
		TextureLookup* newTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(kszDefaultTextureName);
		CAssert(newTextureLookup != nullptr);

		// Get at the first free texture.
		for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
		{
			if (!m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
			{
				// Found one!
				defaultDiffuseTexture = &m_TextureDictionary.m_TextureArray[texIdx];
				break;
			}
		}
		CAssert(defaultDiffuseTexture != nullptr);

		defaultDiffuseTexture->pszTextureName = StringDuplicate(kszDefaultTextureName);
		defaultDiffuseTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultTexturePixels[8][8][4];
		memset(defaultTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
		for (int rowIdx = 0; rowIdx < 8; rowIdx++)
		{
			for (int colIdx = 0; colIdx < 8; colIdx++)
			{
				defaultTexturePixels[rowIdx][colIdx][0] = 0;
				defaultTexturePixels[rowIdx][colIdx][1] = 0;
				defaultTexturePixels[rowIdx][colIdx][2] = ((rowIdx % 2) == 0) ? (((colIdx % 2) == 0) ? 255 : 0) : (((colIdx % 2) == 0) ? 0 : 255);
				defaultTexturePixels[rowIdx][colIdx][3] = 255;
			}
		}
		byte* pPixelData = defaultTexturePixels[0][0];

		defaultDiffuseTexture->width = defaultDiffuseTexture->height = 8;
		if (!g_pRenderer->AcquireTextureResources(*defaultDiffuseTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
		{
			LOG_ERROR("Failed to create default diffuse texture.");
			return false;
		}

		if (newTextureLookup->TextureIndex == kInvalidID_U32)
			newTextureLookup->TextureIndex = texIdx;

		m_pDefaultTextureLookup = newTextureLookup;
	}

	// Default white texture
	{
		Texture* defaultWhiteTexture = nullptr;
		int texIdx = 0;

		// Get an entry into the hashmap.
		TextureLookup* defaultWhiteTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(kszDefaultWhiteTextureName);
		CAssert(defaultWhiteTextureLookup != nullptr);

		// Get at the first free texture.
		for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
		{
			if (!m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
			{
				// Found one!
				defaultWhiteTexture = &m_TextureDictionary.m_TextureArray[texIdx];
				break;
			}
		}
		CAssert(defaultWhiteTexture != nullptr);

		defaultWhiteTexture->pszTextureName = StringDuplicate(kszDefaultWhiteTextureName);
		defaultWhiteTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultTexturePixels[8][8][4];
		memset(defaultTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
		for (int rowIdx = 0; rowIdx < 8; rowIdx++)
		{
			for (int colIdx = 0; colIdx < 8; colIdx++)
			{
				defaultTexturePixels[rowIdx][colIdx][0] = 255;
				defaultTexturePixels[rowIdx][colIdx][1] = 255;
				defaultTexturePixels[rowIdx][colIdx][2] = 255;
				defaultTexturePixels[rowIdx][colIdx][3] = 255;
			}
		}
		byte* pPixelData = defaultTexturePixels[0][0];

		defaultWhiteTexture->width = defaultWhiteTexture->height = 8;
		if (!g_pRenderer->AcquireTextureResources(*defaultWhiteTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
		{
			LOG_ERROR("Failed to create default diffuse texture.");
			return false;
		}

		if (defaultWhiteTextureLookup->TextureIndex == kInvalidID_U32)
			defaultWhiteTextureLookup->TextureIndex = texIdx;

		m_pWhiteTextureLookup = defaultWhiteTextureLookup;
	}

	// Default specular texture
	{
		Texture* pDefaultSpecularTexture = nullptr;
		int texIdx = 0;

		// Get an entry into the hashmap.
		TextureLookup* defaultSpecularTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(kszDefaultSpecularTextureName);
		CAssert(defaultSpecularTextureLookup != nullptr);

		// Get at the first free texture.
		for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
		{
			if (!m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
			{
				// Found one!
				pDefaultSpecularTexture = &m_TextureDictionary.m_TextureArray[texIdx];
				break;
			}
		}
		CAssert(pDefaultSpecularTexture != nullptr);

		pDefaultSpecularTexture->pszTextureName = StringDuplicate(kszDefaultSpecularTextureName);
		pDefaultSpecularTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultSpecularTexturePixels[8][8][4];
		memset(defaultSpecularTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
		for (int rowIdx = 0; rowIdx < 8; rowIdx++)
		{
			for (int colIdx = 0; colIdx < 8; colIdx++)
			{
				defaultSpecularTexturePixels[rowIdx][colIdx][0] = 0;
				defaultSpecularTexturePixels[rowIdx][colIdx][1] = 0;
				defaultSpecularTexturePixels[rowIdx][colIdx][2] = 0;
				defaultSpecularTexturePixels[rowIdx][colIdx][3] = 0;
			}
		}
		byte* pPixelData = defaultSpecularTexturePixels[0][0];

		pDefaultSpecularTexture->width = pDefaultSpecularTexture->height = 8;
		if (!g_pRenderer->AcquireTextureResources(*pDefaultSpecularTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
		{
			LOG_ERROR("Failed to create default specular texture.");
			return false;
		}

		if (defaultSpecularTextureLookup->TextureIndex == kInvalidID_U32)
			defaultSpecularTextureLookup->TextureIndex = texIdx;

		m_pDefaultSpecularTexture = defaultSpecularTextureLookup;
	}

	// Default normal texture
	{
		Texture* pDefaultNormalTexture = nullptr;
		int texIdx = 0;

		// Get an entry into the hashmap.
		TextureLookup* defaultNormalTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(kszDefaultNormalTextureName);
		CAssert(defaultNormalTextureLookup != nullptr);

		// Get at the first free texture.
		for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
		{
			if (!m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
			{
				// Found one!
				pDefaultNormalTexture = &m_TextureDictionary.m_TextureArray[texIdx];
				break;
			}
		}
		CAssert(pDefaultNormalTexture != nullptr);

		pDefaultNormalTexture->pszTextureName = StringDuplicate(kszDefaultNormalTextureName);
		pDefaultNormalTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultNormalTexturePixels[8][8][4];
		memset(defaultNormalTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
		for (int rowIdx = 0; rowIdx < 8; rowIdx++)
		{
			for (int colIdx = 0; colIdx < 8; colIdx++)
			{
				defaultNormalTexturePixels[rowIdx][colIdx][0] = 0;
				defaultNormalTexturePixels[rowIdx][colIdx][1] = 0;
				defaultNormalTexturePixels[rowIdx][colIdx][2] = 128;
				defaultNormalTexturePixels[rowIdx][colIdx][3] = 128;
			}
		}
		byte* pPixelData = defaultNormalTexturePixels[0][0];

		pDefaultNormalTexture->width = pDefaultNormalTexture->height = 8;
		if (!g_pRenderer->AcquireTextureResources(*pDefaultNormalTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
		{
			LOG_ERROR("Failed to create default normal texture.");
			return false;
		}

		if (defaultNormalTextureLookup->TextureIndex == kInvalidID_U32)
			defaultNormalTextureLookup->TextureIndex = texIdx;

		m_pDefaultNormalTexture = defaultNormalTextureLookup;
	}

	return true;
}

//-------------------------------------------------------------------------
// Release default texture resources.
//-------------------------------------------------------------------------
void CTextureSystem::DestroyDefaultTextures()
{
	Texture* pDefaultTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultTextureLookup->TextureIndex];
	g_pRenderer->ReleaseTextureResources(*pDefaultTexture);
	pDefaultTexture->m_pInternalData = nullptr;

	if (pDefaultTexture->pszTextureName)
		StringFree(pDefaultTexture->pszTextureName);

	Texture* pDefaultWhiteTexture = &m_TextureDictionary.m_TextureArray[m_pWhiteTextureLookup->TextureIndex];
	g_pRenderer->ReleaseTextureResources(*pDefaultWhiteTexture);
	pDefaultWhiteTexture->m_pInternalData = nullptr;

	if (pDefaultWhiteTexture->pszTextureName)
		StringFree(pDefaultWhiteTexture->pszTextureName);

	Texture* pDefaultSpecularTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultSpecularTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(*pDefaultSpecularTexture);
	pDefaultSpecularTexture->m_pInternalData = nullptr;

	if (pDefaultSpecularTexture->pszTextureName)
		StringFree(pDefaultSpecularTexture->pszTextureName);


	Texture* pDefaultNormalTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultNormalTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(*pDefaultNormalTexture);
	pDefaultNormalTexture->m_pInternalData = nullptr;

	if (pDefaultNormalTexture->pszTextureName)
		StringFree(pDefaultNormalTexture->pszTextureName);
}