//=============================================================================
// Author: Jared Simons
// Date Created: 8/28/2024
// 
// Purpose: Texture System
//=============================================================================
#include "TextureSystem.h"

#include "Core/Logging.h"
#include "Systems/Renderer.h"

#include <String_Utils.h>

// TODO: Image loader
#define STB_IMAGE_IMPLEMENTATION
#include <Vendor/stb_image.h>

bool TextureSystem::Initialize(void* pSystemConfig, Renderer* pRenderSystem)
{
	if (!pSystemConfig || !pRenderSystem)
	{
		LOG_ERROR("Texture System requires a valid pointer to a Texture System Configuration and Renderer! Texture System creation failed");
		return false;
	}
	m_RenderSystem = pRenderSystem;

	TextureSystemConfiguration* typedConfig = static_cast<TextureSystemConfiguration*>(pSystemConfig);

	if (typedConfig->maxTextureCount < 1)
	{
		LOG_ERROR("Texture System requires that config.maxTextureCount be > 0.");
		return false;
	}
	m_Configuration = *typedConfig;

	m_RegisteredTextures = CHashmap<const char*, Texture>(typedConfig->maxTextureCount);

	LOG_INFO("Texture system initialized (MAX TEXTURES: %d)", typedConfig->maxTextureCount);

	return true;
}

void TextureSystem::Shutdown()
{
	if (m_NumRegisteredTextures > 0)
	{
		LOG_WARN("Texture System shutting down with textures still registered!");
	}
}

bool TextureSystem::LoadTexture(const char* pTexturePath)
{
	if (!pTexturePath)
	{
		LOG_ERROR("TextureSystem::LoadTexture() Requires a string to a texture");
		return false;
	}

	Texture* newTexture = &m_RegisteredTextures[pTexturePath];

	byte* pixelData = stbi_load(pTexturePath, &newTexture->m_Width, &newTexture->m_Height, &newTexture->m_ChannelCount, 4); // NOTE: For now requiring 4 channels

	if (!pixelData)
	{
		LOG_ERROR("LoadTexture() -> Failed to obtain pixel data - Texture creation failed.");
		return false;
	}

	newTexture->m_TextureName = StringDuplicate(pTexturePath); // TODO: Strip texture name down to just the file name...

	// Obtain GPU Texture resources
	m_RenderSystem->CreateTextureResources(*newTexture, pixelData);

	// Free resources that are no longer needed.
	stbi_image_free(pixelData);

	m_NumRegisteredTextures++;

	LOG_DEBUG("Loaded texture '%s'.", pTexturePath);

	return true;
}

void TextureSystem::UnloadTexture(Texture& pTexture)
{
	// Release GPU Texture resources
	m_RenderSystem->DestroyTextureResources(pTexture);

	pTexture.m_Width = pTexture.m_Height = pTexture.m_ChannelCount = 0;
	pTexture.m_TextureName = nullptr;
	pTexture.m_internalData = nullptr;

	m_NumRegisteredTextures--;
}

Texture* TextureSystem::AcquireTexture(const char* pTexturePath)
{
	if (!pTexturePath)
	{
		LOG_ERROR("AcquireTexture() Requires a string to a texture.");
		return nullptr; // TODO: Return default texture?
	}

	Texture* pTexture = &m_RegisteredTextures[pTexturePath];
	if (!pTexture->m_internalData)
	{
		// Load the texture if it hasn't yet been loaded
		LoadTexture(pTexturePath);
	}

	// TODO: Handle reference counting

	return pTexture;
}

static TextureSystem* s_TextureSystem = nullptr;
TextureSystem* gpTextureSystem = s_TextureSystem;
