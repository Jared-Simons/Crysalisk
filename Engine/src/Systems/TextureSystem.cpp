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
#include <Resources/Loaders/ImageLoader.h>

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
	
	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName, bool bAutoRelease) override;
	virtual void				ReleaseTexture(TextureHandle_t hTexture) override;

	virtual uint32_t			GetTextureFlags(TextureHandle_t hTexture) const override;

	virtual TextureHandle_t		WrapTextureInternal(const char* pszTextureName, size_t iTexWidth, size_t iTexHeight, byte* pPixelData) override;

			// This variant exists to free textures in the array without having to access the hashmap from the calling function. This really only exists to easily clear the textures on shutdown.
			void				ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease);

	virtual int32_t				GetTextureWidth(TextureHandle_t hTexture) const override;
	virtual int32_t				GetTextureHeight(TextureHandle_t hTexture) const override;

	// Use this to convert a texture handle to a texture pointer. This is necessary so the renderer can get at texture resources without relying on the texture system.
	// Nothing except the renderer should be using this, handles are the right answer elsewhere.
	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) override { return &m_TextureDictionary.m_TextureArray[hTex]; }

	virtual TextureHandle_t		GetDefaultDiffuseTexture() override { return m_pDefaultTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultWhiteTexture() override { return m_pWhiteTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultSpecularTexture() override { return m_pDefaultSpecularTexture->TextureIndex; }
	virtual TextureHandle_t		GetDefaultNormalTexture() override { return m_pDefaultNormalTexture->TextureIndex; }

private:

	TextureHandle_t				GetFirstFreeTextureHandle() const;
	void						ClearTextureDictionaryEntry(const char* pszTextureName);

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
ITextureSystem* g_pTextureSystem_Internal = &s_TextureSystem;

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
TextureHandle_t CTextureSystem::AcquireTexture(const char* pszTextureName, bool bAutoRelease)
{
	ImageLoader::ImageResource_t	imageResource{};
	TextureHandle_t					newTextureHandle = kInvalidID_U32;
	TextureLookup*					pTextureLookup = nullptr;
	Texture*						pTexture = nullptr;

	if (!pszTextureName)
	{
		LOG_ERROR("AcquireTexture() was called with null texture name! Returning default texture.");
		return m_pDefaultTextureLookup->TextureIndex;
	}

	// Check to see if the texture is already loaded.
	pTextureLookup = &m_TextureDictionary.m_TextureHashmap[pszTextureName];
	if (pTextureLookup->TextureIndex == kInvalidID_U32)
	{
		// Texture data does not exist yet, get that going.

		// Load up the image resource from disk.
		// Note that these get released in imageResource's destructor.
		if (!ImageLoader::LoadImage(pszTextureName, imageResource))
		{
			LOG_ERROR("Failed to load texture. Returning default texture.");
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Get the next free slot in the texture dictionary.
		newTextureHandle = GetFirstFreeTextureHandle();
		if (newTextureHandle == kInvalidID_U32)
		{
			LOG_ERROR("AcquireTexture() could not acquire a new texture slot. Increase texture system config to allow for more.");
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Set up our hashmap entry.
		pTextureLookup->AutoRelease = bAutoRelease;
		pTextureLookup->ReferenceCount = 0; // We'll increment this further down in the function.
		pTextureLookup->TextureIndex = newTextureHandle;

		// Finally, use the loaded resource to set up our texture entry.
		pTexture = &m_TextureDictionary.m_TextureArray[newTextureHandle];
		CAssert(pTexture);

		pTexture->width = imageResource.Width;
		pTexture->height = imageResource.Height;
		pTexture->numChannels = imageResource.NumChannels; // TODO: Use texture format here instead?
		pTexture->textureFlags |= imageResource.bIsTransparent ? TEXTURE_FLAG_IS_TRANSPARENT : 0;
		pTexture->pszTextureName = StringDuplicate(pszTextureName);

		// Acquire renderer resources and upload to the GPU.
		if (!g_pRenderer->AcquireTextureResources(pTexture, newTextureHandle, (pTexture->width * pTexture->height * pTexture->numChannels), imageResource.pPixelData))
		{
			LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Texture acquisition failed!", pszTextureName);
			
			// Clear out texture/hashmap entries.
			ClearTextureDictionaryEntry(pszTextureName);
			return m_pDefaultTextureLookup->TextureIndex;
		}

		// Increment the reference count.
		pTextureLookup->ReferenceCount++;
	}
	else
	{
		// The texture's resources already exist, just increment the reference count and return the handle.
		pTextureLookup->ReferenceCount++;
		newTextureHandle = pTextureLookup->TextureIndex;
	}

	return newTextureHandle;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTexture(TextureHandle_t hTexture)
{
	CAssert(hTexture != kInvalidID_U32);
	
	Texture*		pTexture = &m_TextureDictionary.m_TextureArray[hTexture];
	TextureLookup*	pLookup = &m_TextureDictionary.m_TextureHashmap[pTexture->pszTextureName];
	CAssert(hTexture == pLookup->TextureIndex);	// These need to line up...

	// Decrement the reference count.
	pLookup->ReferenceCount--;

	// Release data, if conditions are met.
	CAssert(pLookup->ReferenceCount >= 0);
	if (pLookup->ReferenceCount == 0 && pLookup->AutoRelease == true)
	{
		// Release internal texture data.
		g_pRenderer->ReleaseTextureResources(pTexture);

		// Clear the hashmap and texture data.
		ClearTextureDictionaryEntry(pTexture->pszTextureName);
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
uint32_t CTextureSystem::GetTextureFlags(TextureHandle_t hTexture) const
{
	CAssert(hTexture != kInvalidID_U32);
	Texture* pTexture = &m_TextureDictionary.m_TextureArray[hTexture];

	return pTexture->textureFlags;
}

//-------------------------------------------------------------------------
// Takes already created pixel data and wraps it in a texture object.
// This returns a handle to that texture.
//-------------------------------------------------------------------------
TextureHandle_t CTextureSystem::WrapTextureInternal(const char* pszTextureName, size_t iTexWidth, size_t iTexHeight, byte* pPixelData)
{
	Texture* newTexture = nullptr;
	int texIdx = 0;

	// Make sure our state is ready to go.
	if (!m_TextureDictionary.m_TextureArray)
	{
		LOG_WARN("CTextureSystem::WrapTextureInternal() called before texture system was initialized! Returning NULL handle.");
		return 0; // TODO: Invalid ID.
	}

	if (!pszTextureName)
	{
		LOG_ERROR("CTextureSystem::WrapTextureInternal() requires a path to the texture to load! Returning default texture.");
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
		texIdx = GetFirstFreeTextureHandle();
		newTexture = &m_TextureDictionary.m_TextureArray[texIdx];

		if (!newTexture)
		{
			LOG_ERROR("Could not load texture '%s' because all texture slots were full, returning default texture.", pszTextureName);
			return m_pDefaultTextureLookup->TextureIndex;
		}
		newTexture->width = iTexWidth;
		newTexture->height = iTexHeight;
		newTexture->numChannels = 1; // TODO: Configurable.

		// Send the data off to the renderer to be uploaded to the GPU.
		if (!g_pRenderer->AcquireTextureResources(newTexture, texIdx, (newTexture->width * newTexture->height * newTexture->numChannels), pPixelData))
		{
			LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Returning default texture.", pszTextureName);
			return m_pDefaultTextureLookup->TextureIndex;
		}

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
// Same as above, except this goes right to the texture array, skipping the lookup in the hashmap.
// This is used so that we don't need to get the table lookup on shutdown, when we just want to release all texture resources anyways.
// NOTE THAT THIS WILL FREE RESOURCES WITH NO REGARD TO REF. COUNTING!!!
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease)
{
	// Default textures do not go through this pipe.
	if (pTexture.textureFlags & TEXTURE_FLAG_DEFAULT_TEXTURE)
		return;

	g_pRenderer->ReleaseTextureResources(&pTexture);
	if (pTexture.pszTextureName)
		StringFree(pTexture.pszTextureName);

	pTexture.m_pInternalData = nullptr;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
int32_t CTextureSystem::GetTextureWidth(TextureHandle_t hTexture) const
{
	return m_TextureDictionary.m_TextureArray[hTexture].width;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
int32_t CTextureSystem::GetTextureHeight(TextureHandle_t hTexture) const
{
	return m_TextureDictionary.m_TextureArray[hTexture].height;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
TextureHandle_t CTextureSystem::GetFirstFreeTextureHandle() const
{
	if (!s_TextureSystem.m_TextureDictionary.m_TextureArray)
	{
		LOG_ERROR("Attempting to acquire a texture prior to texture system initialization, texture acquisition failed.");
		return kInvalidID_U32;
	}

	for (int texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
	{
		if (!s_TextureSystem.m_TextureDictionary.m_TextureArray[texIdx].m_pInternalData)
		{
			// Found a free slot, return!
			return texIdx;
		}
	}

	LOG_WARN("Failed to find free texture slot in the texture dictionary, returning default texture.");
	return kInvalidID_U32;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CTextureSystem::ClearTextureDictionaryEntry(const char* pszTextureName)
{
	CAssert(pszTextureName);

	TextureLookup*	pLookup = &m_TextureDictionary.m_TextureHashmap[pszTextureName];
	Texture*		pTexture = &m_TextureDictionary.m_TextureArray[pLookup->TextureIndex];

	CAssert(pLookup && pTexture);

	pTexture->width = pTexture->height = pTexture->numChannels = 0;
	pTexture->textureFlags = 0;
	if (pTexture->pszTextureName)
		StringFree(pTexture->pszTextureName);

	if (pTexture->m_pInternalData)
	{
		// TODO: Free any internal resources that may be leftover.
	}

	pLookup->AutoRelease = false;
	pLookup->ReferenceCount = 0;
	pLookup->TextureIndex = kInvalidID_U32;
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

		texIdx = GetFirstFreeTextureHandle();
		defaultDiffuseTexture = &m_TextureDictionary.m_TextureArray[texIdx];
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
		defaultDiffuseTexture->numChannels = 4;
		if (!g_pRenderer->AcquireTextureResources(defaultDiffuseTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
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
		texIdx = GetFirstFreeTextureHandle();
		defaultWhiteTexture = &m_TextureDictionary.m_TextureArray[texIdx];
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
		defaultWhiteTexture->numChannels = 4;
		if (!g_pRenderer->AcquireTextureResources(defaultWhiteTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
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
		texIdx = GetFirstFreeTextureHandle();
		pDefaultSpecularTexture = &m_TextureDictionary.m_TextureArray[texIdx];
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
		pDefaultSpecularTexture->numChannels = 4;
		if (!g_pRenderer->AcquireTextureResources(pDefaultSpecularTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
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
		texIdx = GetFirstFreeTextureHandle();
		pDefaultNormalTexture = &m_TextureDictionary.m_TextureArray[texIdx];
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
		pDefaultNormalTexture->numChannels = 4;
		if (!g_pRenderer->AcquireTextureResources(pDefaultNormalTexture, texIdx, sizeof(byte) * 8 * 8 * 4, pPixelData))
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
	g_pRenderer->ReleaseTextureResources(pDefaultTexture);
	pDefaultTexture->m_pInternalData = nullptr;

	if (pDefaultTexture->pszTextureName)
		StringFree(pDefaultTexture->pszTextureName);

	Texture* pDefaultWhiteTexture = &m_TextureDictionary.m_TextureArray[m_pWhiteTextureLookup->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultWhiteTexture);
	pDefaultWhiteTexture->m_pInternalData = nullptr;

	if (pDefaultWhiteTexture->pszTextureName)
		StringFree(pDefaultWhiteTexture->pszTextureName);

	Texture* pDefaultSpecularTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultSpecularTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultSpecularTexture);
	pDefaultSpecularTexture->m_pInternalData = nullptr;

	if (pDefaultSpecularTexture->pszTextureName)
		StringFree(pDefaultSpecularTexture->pszTextureName);


	Texture* pDefaultNormalTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultNormalTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultNormalTexture);
	pDefaultNormalTexture->m_pInternalData = nullptr;

	if (pDefaultNormalTexture->pszTextureName)
		StringFree(pDefaultNormalTexture->pszTextureName);
}