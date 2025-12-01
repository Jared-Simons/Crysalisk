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
#include "Systems/JobSystem.h"

//-------------------------------------------------------------------------
// Texture System Implementation.
//-------------------------------------------------------------------------
class CTextureSystem : public ITextureSystem
{
public:
	// Initialization, shutdown.
	virtual bool				Initialize(TextureSystemConfig_t config) override;
	virtual void				Shutdown() override;
	
	// Acquires the resources for the provided texture.
	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName, bool bAutoRelease) override;
	// Acquires a texture given an existing resource.
	virtual TextureHandle_t		AcquireTextureFromResource(ImageLoader::ImageResource_t& pResource, const char* pszTextureName, bool bAutoRelease) override;
	// Releases a reference to an existing texture.
	virtual void				ReleaseTexture(TextureHandle_t hTexture) override;

	// Returns the state of a specific texture flag(s).
	virtual bool				IsTextureFlagSet(TextureHandle_t hTexture, TextureFlags_t flagsToCheck) const override;

	// TODO: Implement this and comment.
	virtual TextureHandle_t		WrapTextureInternal(const char* pszTextureName, int iTexWidth, int iTexHeight, byte* pPixelData) override;

			// This variant exists to free textures in the array without having to access the hashmap from the calling function. This really only exists to easily clear the textures on shutdown.
			void				ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease);

	// Returns a texture's width/height.
	virtual int32_t				GetTextureWidth(TextureHandle_t hTexture) const override;
	virtual int32_t				GetTextureHeight(TextureHandle_t hTexture) const override;

	// Use this to convert a texture handle to a texture pointer. This is necessary so the renderer can get at texture resources without relying on the texture system.
	// Nothing except the renderer should be using this, handles are the right answer elsewhere.
	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) override { return &m_TextureDictionary.m_TextureArray[hTex]; }

	// Getters for default textures.
	virtual TextureHandle_t		GetDefaultDiffuseTexture() override { return m_pDefaultTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultWhiteTexture() override { return m_pWhiteTextureLookup->TextureIndex; }
	virtual TextureHandle_t		GetDefaultSpecularTexture() override { return m_pDefaultSpecularTexture->TextureIndex; }
	virtual TextureHandle_t		GetDefaultNormalTexture() override { return m_pDefaultNormalTexture->TextureIndex; }

private:
	// Returns the first available handle in the texture dictionary.
	TextureHandle_t				GetFirstFreeTextureHandle() const;

	// Clears a given texture dictionary entry.
	void						ClearTextureDictionaryEntry(const char* pszTextureName);

	// Create/Destroy default textures.
	bool						CreateDefaultTextures();
	void						DestroyDefaultTextures();

private:
	// The texture system configuration passed in from the application configuration.
	TextureSystemConfig_t	m_Config{};

	// A lookup for a particular texture.
	struct TextureLookup
	{
		// The internal handle for the texture.
		TextureHandle_t	TextureIndex{ kInvalidID_U32 };
		// The current reference count of the texture.
		int				ReferenceCount{ 0 };
		// Indicates if this texture should automatically release it's resources when it's reference count reaches 0.
		// NOTE: This can only be set when first acquiring the texture.
		bool			AutoRelease{ false };
	};

	struct TextureDictionary
	{
		// Allows for string-based lookups for texture.
		Utl::CStringHashMap<TextureLookup>			m_TextureHashmap{};
		// An array of textures.
		Texture*									m_TextureArray{ nullptr };
	};

	// The texture dictionary.
	TextureDictionary	m_TextureDictionary{};

	// Default textures.
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

	// THREADTODO: TEST!!
	{
		ImageLoader::ImageResource_t	 textureResource{};
		ImageLoader::ImageLoaderParams_t textureParams{};

		textureParams.pszImageName = Utl::StringDuplicate("TestDevTexture.png");
		textureParams.pOutResource = &textureResource;

		JobDeclaration_t textureLoadJob{};
		textureLoadJob.pfnEntryPoint = ImageLoader::LoadImage_Threaded;
		textureLoadJob.pParams = &textureParams;
		textureLoadJob.iParamSize = sizeof(ImageLoader::ImageLoaderParams_t);
		textureLoadJob.iJobType = WORKER_THREAD_TYPE_DISK_LOADING;
		textureLoadJob.ePriority = eJobPriority::NORMAL;

		g_pJobSystem->SubmitJob(textureLoadJob);

		// The job system takes a copy of the resource params, so it's okay to clean up the original copy here.
		//Utl::StringFree((char*)textureParams.pszImageName);
	}
	// END TEST!!

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
		if (m_TextureDictionary.m_TextureArray[idx].internalHandle != kInvalidID_U32)
		{
			ReleaseTextureAtIndex(m_TextureDictionary.m_TextureArray[idx], true); // True to force the texture system to release internal resources.
		}
	}

	// Release default textures.
	DestroyDefaultTextures();

	delete[] m_TextureDictionary.m_TextureArray;
}

//-------------------------------------------------------------------------
// Purpose: Attempts to acquire the resources for a given texture.
// Input:	pszTextureName - The texture to acquire resources for.
//			bAutoRelease - Indicates if the texture's resources should be released when its reference count reaches 0.
//				NOTE: This only gets set when the texture is first acquired.
// Output:	Returns a handle to the new texture.
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
	pTextureLookup = m_TextureDictionary.m_TextureHashmap.Insert(pszTextureName);
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

		newTextureHandle = AcquireTextureFromResource(imageResource, pszTextureName, bAutoRelease);
	}
	else
	{
		// The texture's resources already exist, just increment the reference count and return the handle.
		pTextureLookup->ReferenceCount++;
		newTextureHandle = pTextureLookup->TextureIndex;
	}

	CAssert(newTextureHandle == pTextureLookup->TextureIndex);
	return newTextureHandle;
}

//-------------------------------------------------------------------------
// Purpose: Acquires a texture given an existing resource.
// Input:	pszTextureName - The texture to acquire resources for.
//			pResource - The existing resource data to use for the new texture.
//			bAutoRelease - Indicates if the texture's resources should be released when its reference count reaches 0.
//				NOTE: This only gets set when the texture is first acquired.
// Output:	Returns a handle to the new texture.
//-------------------------------------------------------------------------
TextureHandle_t CTextureSystem::AcquireTextureFromResource(ImageLoader::ImageResource_t& pResource, const char* pszTextureName, bool bAutoRelease)
{
	TextureLookup*					pTextureLookup = &m_TextureDictionary.m_TextureHashmap[pszTextureName];
	Texture*						pTexture = nullptr;

	// Get the next free slot in the texture dictionary.
	TextureHandle_t newTextureHandle = GetFirstFreeTextureHandle();
	if (newTextureHandle == kInvalidID_U32)
	{
		LOG_ERROR("AcquireTexture() could not acquire a new texture slot. Increase texture system config to allow for more.");
		return m_pDefaultTextureLookup->TextureIndex;
	}

	pTexture = &m_TextureDictionary.m_TextureArray[newTextureHandle];

	// Set up our hashmap entry.
	pTextureLookup->AutoRelease = bAutoRelease;
	pTextureLookup->ReferenceCount = 0; // We'll increment this further down in the function.
	pTextureLookup->TextureIndex = newTextureHandle;

	// Finally, use the loaded resource to set up our texture entry.
	pTexture = &m_TextureDictionary.m_TextureArray[newTextureHandle];
	CAssert(pTexture);

	pTexture->width = pResource.Width;
	pTexture->height = pResource.Height;
	pTexture->numChannels = pResource.NumChannels; // TODO: Use texture format here instead?
	pTexture->textureFlags |= pResource.bIsTransparent ? TEXTURE_FLAG_IS_TRANSPARENT : 0;
	pTexture->pszTextureName = Utl::StringDuplicate(pszTextureName);

	// Acquire renderer resources and upload to the GPU.
	if (!g_pRenderer->AcquireTextureResources(pTexture, newTextureHandle, (pTexture->width * pTexture->height * pTexture->numChannels), pResource.pPixelData))
	{
		LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Texture acquisition failed!", pszTextureName);

		// Clear out texture/hashmap entries.
		ClearTextureDictionaryEntry(pszTextureName);
		return m_pDefaultTextureLookup->TextureIndex;
	}

	// Increment the reference count.
	pTextureLookup->ReferenceCount++;

	return newTextureHandle;
}

//-------------------------------------------------------------------------
// Purpose: Releases an existing reference to a texture.
// Input:	hTexture - The texture to release a reference to.
// Output:	none.
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTexture(TextureHandle_t hTexture)
{
	CAssert(hTexture != kInvalidID_U32);
	
	Texture*		pTexture = &m_TextureDictionary.m_TextureArray[hTexture];
	TextureLookup*	pLookup = &m_TextureDictionary.m_TextureHashmap[pTexture->pszTextureName];
	CAssert(hTexture == pLookup->TextureIndex);	// These need to line up...

	// Do absolutely nothing with default textures.
	// They need to stick around until the end.
	if (pTexture->internalHandle == m_pDefaultTextureLookup->TextureIndex
		|| pTexture->internalHandle == m_pDefaultSpecularTexture->TextureIndex
		|| pTexture->internalHandle == m_pDefaultNormalTexture->TextureIndex)
	{
		return;
	}

	// Decrement the reference count.
	pLookup->ReferenceCount--;

	// Release data, if conditions are met.
	CAssert(pLookup->ReferenceCount >= 0);
	if (pLookup->ReferenceCount == 0 && pLookup->AutoRelease == true)
	{
		// Release internal texture data.
		g_pRenderer->ReleaseTextureResources(pTexture);
		pTexture->internalHandle = kInvalidID_U32;

		// Clear the hashmap and texture data.
		ClearTextureDictionaryEntry(pTexture->pszTextureName);
	}
}

//-------------------------------------------------------------------------
// Purpose: Returns the state of a specific texture flag(s).
// Input:	hTexture - The texture whose flags to check.
//			flagsToCheck - The flag(s) to check the state of.
// Output:	True if the flag(s) are set, otherwise false.
//-------------------------------------------------------------------------
bool CTextureSystem::IsTextureFlagSet(TextureHandle_t hTexture, TextureFlags_t flagsToCheck) const
{
	CAssert(hTexture != kInvalidID_U32);
	Texture* pTexture = &m_TextureDictionary.m_TextureArray[hTexture];

	return (pTexture->textureFlags & flagsToCheck) != 0;
}

//-------------------------------------------------------------------------
// Takes already created pixel data and wraps it in a texture object.
// This returns a handle to that texture.
//-------------------------------------------------------------------------
TextureHandle_t CTextureSystem::WrapTextureInternal(const char* pszTextureName, int iTexWidth, int iTexHeight, byte* pPixelData)
{
	// TODO: IMPLEMENT THIS!
	CAssert(0);
	return 0;
}

//-------------------------------------------------------------------------
// Purpose: Releases an existing texture, without care for reference counting.
// Input:	pTexture - The internal texture to release resources for.
//			bForceRelease - TODO: This currently does nothing...
// Output:	none.
//-------------------------------------------------------------------------
void CTextureSystem::ReleaseTextureAtIndex(Texture& pTexture, bool bForceRelease)
{
	// Default textures do not go through this pipe.
	if (pTexture.textureFlags & TEXTURE_FLAG_DEFAULT_TEXTURE)
		return;

	g_pRenderer->ReleaseTextureResources(&pTexture);
	if (pTexture.pszTextureName)
		Utl::StringFree(pTexture.pszTextureName);

	pTexture.internalHandle = kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Returns a texture's width.
// Input:	hTexture - The texture whose width to return.
// Output:	Returns the texture's width.
//-------------------------------------------------------------------------
int32_t CTextureSystem::GetTextureWidth(TextureHandle_t hTexture) const
{
	return m_TextureDictionary.m_TextureArray[hTexture].width;
}

//-------------------------------------------------------------------------
// Purpose: Returns a texture's height.
// Input:	hTexture - The texture whose height to return.
// Output:	Returns the texture's height.
//-------------------------------------------------------------------------
int32_t CTextureSystem::GetTextureHeight(TextureHandle_t hTexture) const
{
	return m_TextureDictionary.m_TextureArray[hTexture].height;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available handle in the texture dictionary.
// Input:	none.
// Output:	Returns the first available texture handle.
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
		if (s_TextureSystem.m_TextureDictionary.m_TextureArray[texIdx].internalHandle == kInvalidID_U32)
		{
			// Found a free slot, return!
			return texIdx;
		}
	}

	LOG_WARN("Failed to find free texture slot in the texture dictionary, returning default texture.");
	return kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Clears a given texture dictionary entry.
// Input:	pszTextureName - The name of the texture whose entry to vacate.
// Output:	none.
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
		Utl::StringFree(pTexture->pszTextureName);

	if (pTexture->internalHandle != kInvalidID_U32)
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

		defaultDiffuseTexture->pszTextureName = Utl::StringDuplicate(kszDefaultTextureName);
		defaultDiffuseTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultTexturePixels[8][8][4];
		Platform::MemorySet(defaultTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
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

		defaultWhiteTexture->pszTextureName = Utl::StringDuplicate(kszDefaultWhiteTextureName);
		defaultWhiteTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultTexturePixels[8][8][4];
		Platform::MemorySet(defaultTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
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

		pDefaultSpecularTexture->pszTextureName = Utl::StringDuplicate(kszDefaultSpecularTextureName);
		pDefaultSpecularTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultSpecularTexturePixels[8][8][4];
		Platform::MemorySet(defaultSpecularTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
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

		pDefaultNormalTexture->pszTextureName = Utl::StringDuplicate(kszDefaultNormalTextureName);
		pDefaultNormalTexture->textureFlags = TEXTURE_FLAG_DEFAULT_TEXTURE;

		byte	defaultNormalTexturePixels[8][8][4];
		Platform::MemorySet(defaultNormalTexturePixels, 0, sizeof(byte) * 8 * 8 * 4);
		for (int rowIdx = 0; rowIdx < 8; rowIdx++)
		{
			for (int colIdx = 0; colIdx < 8; colIdx++)
			{
				defaultNormalTexturePixels[rowIdx][colIdx][0] = 128;
				defaultNormalTexturePixels[rowIdx][colIdx][1] = 128;
				defaultNormalTexturePixels[rowIdx][colIdx][2] = 255;
				defaultNormalTexturePixels[rowIdx][colIdx][3] = 255;
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
	pDefaultTexture->internalHandle = kInvalidID_U32;

	if (pDefaultTexture->pszTextureName)
		Utl::StringFree(pDefaultTexture->pszTextureName);

	Texture* pDefaultWhiteTexture = &m_TextureDictionary.m_TextureArray[m_pWhiteTextureLookup->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultWhiteTexture);
	pDefaultWhiteTexture->internalHandle = kInvalidID_U32;

	if (pDefaultWhiteTexture->pszTextureName)
		Utl::StringFree(pDefaultWhiteTexture->pszTextureName);

	Texture* pDefaultSpecularTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultSpecularTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultSpecularTexture);
	pDefaultSpecularTexture->internalHandle = kInvalidID_U32;

	if (pDefaultSpecularTexture->pszTextureName)
		Utl::StringFree(pDefaultSpecularTexture->pszTextureName);


	Texture* pDefaultNormalTexture = &m_TextureDictionary.m_TextureArray[m_pDefaultNormalTexture->TextureIndex];
	g_pRenderer->ReleaseTextureResources(pDefaultNormalTexture);
	pDefaultNormalTexture->internalHandle = kInvalidID_U32;

	if (pDefaultNormalTexture->pszTextureName)
		Utl::StringFree(pDefaultNormalTexture->pszTextureName);
}