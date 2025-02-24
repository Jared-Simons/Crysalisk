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

	virtual TextureHandle_t	LoadTexture(const char* pszTextureName) override;
	virtual void			ReleaseTexture(TextureHandle_t textureHandle) override;

private:
	TextureSystemConfig_t	m_Config{};
	Utl::CHashMap<const char*, TextureHandle_t>	m_TextureHashmap{};
	Texture* m_TextureArray{ nullptr };
};

static CTextureSystem s_TextureSystem;
ITextureSystem* g_pTextureSystem = &s_TextureSystem;

bool CTextureSystem::Initialize(TextureSystemConfig_t config)
{
	m_Config = config;
	m_TextureHashmap.Resize(m_Config.MaxTextures);

	// Allocate memory for our texture array.
	m_TextureArray = (Texture*)g_pMemoryManager->Allocate(sizeof(Texture) * m_Config.MaxTextures);

	LOG_INFO("Texture system initialized! (Max textures: %d)", config.MaxTextures);

	return true;
}

void CTextureSystem::Shutdown()
{
	g_pMemoryManager->Free(m_TextureArray);
}

TextureHandle_t CTextureSystem::LoadTexture(const char* pszTextureName)
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
		return 0; // TODO: Default.
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
		return 0; // TODO: Default texture.
	}

	// Load up the image
	byte* pPixelData = stbi_load(pszTextureName, &newTexture->width, &newTexture->height, &newTexture->numChannels, 4);

	if (!pPixelData)
	{
		LOG_ERROR("Failed to load texture '%s'. Returning default texture.", pszTextureName); // TODO: Failure reason?
		return 0; // TODO: Default texture.
	}

	// Send the data off to the renderer to be uploaded to the GPU.
	if (!g_pRenderer->AcquireTextureResources(*newTexture, (newTexture->width * newTexture->height * newTexture->numChannels), pPixelData))
	{
		LOG_ERROR("Failed to acquire renderer resources for texture '%s'. Returning default texture.", pszTextureName);
		stbi_image_free(pPixelData);
		return 0; // TODO: Defaulte texture.
	}

	// Free the original pixel data.
	stbi_image_free(pPixelData);

	LOG_VERBOSE("Successfully loaded texture: '%s'!", pszTextureName);
	return texIdx; // TODO: Work this with the texture hash.
}

void CTextureSystem::ReleaseTexture(TextureHandle_t)
{
	// TODO: Implement.
}