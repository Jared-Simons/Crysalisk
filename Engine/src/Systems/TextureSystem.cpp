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

class CTextureSystem : public ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual Texture*		LoadTexture(const char* pszTextureName) override;
	virtual void			ReleaseTexture(Texture& pTexture) override;

	bool			CreateDefaultTextures();

private:
	TextureSystemConfig_t	m_Config{};
	Utl::CHashMap<const char*, TextureHandle_t>	m_TextureHashmap{};
	Texture* m_TextureArray{ nullptr };

	// Default textures:
	Texture m_DefaultDiffuseTexture;
};

static CTextureSystem s_TextureSystem;
ITextureSystem* g_pTextureSystem = &s_TextureSystem;

bool CTextureSystem::Initialize(TextureSystemConfig_t config)
{
	m_Config = config;
	m_TextureHashmap.Resize(m_Config.MaxTextures);

	// Allocate memory for our texture array.
	m_TextureArray = (Texture*)g_pMemoryManager->Allocate(sizeof(Texture) * m_Config.MaxTextures);

	if (!CreateDefaultTextures())
	{
		LOG_ERROR("Failed to create default textures!");
		return false;
	}

	LOG_INFO("Texture system initialized! (Max textures: %d)", config.MaxTextures);

	return true;
}

void CTextureSystem::Shutdown()
{
	// Free any texture resources in use by the renderer.
	for (int idx = 0; idx < m_Config.MaxTextures; idx++)
	{
		if (m_TextureArray[idx].m_pInternalData != nullptr)
		{
			ReleaseTexture(m_TextureArray[idx]);
			m_TextureArray[idx].m_pInternalData = nullptr;
		}
	}

	// Release default textures.
	ReleaseTexture(m_DefaultDiffuseTexture);

	g_pMemoryManager->Free(m_TextureArray);
}

Texture* CTextureSystem::LoadTexture(const char* pszTextureName)
{
	Texture* newTexture = nullptr;
	int texIdx = 0;

	// Make sure our state is ready to go.
	if (!m_TextureArray)
	{
		LOG_WARN("CTextureSystem::LoadTexture() called before texture system was initialized! Returning NULL handle.");
		return 0; // TODO: Invalid ID.
	}

	if (!pszTextureName)
	{
		LOG_ERROR("CTextureSystem::LoadTexture() requires a path to the texture to load! Returning default texture.");
		return &m_DefaultDiffuseTexture;
	}

	// Get at the first free texture.
	for (texIdx = 0; texIdx < m_Config.MaxTextures; texIdx++)
	{
		if (!m_TextureArray[texIdx].m_pInternalData)
		{
			// Found one!
			newTexture = &m_TextureArray[texIdx];
			break;
		}
	}

	if (!newTexture)
	{
		LOG_ERROR("Could not load texture '%s' because all texture slots were full, returning default texture.", pszTextureName);
		return &m_DefaultDiffuseTexture;
	}

	// Load up the image
	byte* pPixelData = stbi_load(pszTextureName, &newTexture->width, &newTexture->height, &newTexture->numChannels, 4);

	if (!pPixelData)
	{
		LOG_ERROR("Failed to load texture '%s'. Returning default texture.", pszTextureName); // TODO: Failure reason?
		return &m_DefaultDiffuseTexture; 
	}

	// Send the data off to the renderer to be uploaded to the GPU.
	if (!g_pRenderer->AcquireTextureResources(*newTexture, (newTexture->width * newTexture->height * newTexture->numChannels), pPixelData))
	{
		LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Returning default texture.", pszTextureName);
		stbi_image_free(pPixelData);
		return &m_DefaultDiffuseTexture;
	}

	// Free the original pixel data.
	stbi_image_free(pPixelData);

	TextureHandle_t newTextureHash = m_TextureHashmap.Insert(pszTextureName);

	LOG_VERBOSE("Successfully loaded texture: '%s', at index: %d", pszTextureName, newTextureHash);
	return newTexture; // TODO: Work this with the texture hash.
}

void CTextureSystem::ReleaseTexture(Texture& pTexture)
{
	// TODO: Implement.
	g_pRenderer->ReleaseTextureResources(pTexture);
}

bool CTextureSystem::CreateDefaultTextures()
{
	LOG_INFO("Creating default textures");

	// Default diffuse texture
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

	m_DefaultDiffuseTexture.width = m_DefaultDiffuseTexture.height = 8;
	if (!g_pRenderer->AcquireTextureResources(m_DefaultDiffuseTexture, sizeof(byte) * 8 * 8 * 4, pPixelData))
	{
		LOG_ERROR("Failed to create default diffuse texture.");
		return false;
	}

	return true;
}