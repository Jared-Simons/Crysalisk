//=============================================================================
// Author: Jared Simons
// Date Created: 8/28/2024
// 
// Purpose: Texture System
//=============================================================================
#include "TextureSystem.h"

#include "Core/Logging.h"
#include "Systems/IRenderer.h"

#include <String_Utils.h>

// TODO: Image loader
#define STB_IMAGE_IMPLEMENTATION
#include <Vendor/stb_image.h>

bool TextureSystem::Initialize(void* pSystemConfig)
{
	if (!pSystemConfig)
	{
		LOG_ERROR("Texture System requires a valid pointer to a Texture System Configuration! Texture System creation failed");
		return false;
	}

	TextureSystemConfiguration* typedConfig = static_cast<TextureSystemConfiguration*>(pSystemConfig);

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

	unsigned char* pixelData = stbi_load(pTexturePath, &newTexture->m_Width, &newTexture->m_Height, &newTexture->m_ChannelCount, 4); // NOTE: For now requiring 4 channels

	if (!pixelData)
	{
		LOG_ERROR("LoadTexture() -> Failed to obtain pixel data - Texture creation failed.");
		return false;
	}

	newTexture->m_TextureName = StringDuplicate(pTexturePath); // TODO: Strip texture name down to just the file name...

	// Obtain GPU Texture resources
	gpRenderer->CreateTextureResources(*newTexture, pixelData);

	// Free resources that are no longer needed.
	stbi_image_free(pixelData);

	m_NumRegisteredTextures++;

	LOG_DEBUG("Loaded texture '%s'.", pTexturePath);

	return true;
}

Texture* TextureSystem::AcquireTexture(const char* pTexturePath)
{
	if (!pTexturePath)
	{
		LOG_ERROR("AcquireTexture() Requires a string to a texture.");
		return nullptr; // TODO: Return default texture?
	}

	Texture* pTexture = &m_RegisteredTextures[pTexturePath];
	if (!pTexture->m_InternalData)
	{
		// TODO: Load the texture if it hasn't yet been loaded
		LOG_WARN("AcquireTexture does not yet load textures... IMPLEMENT!");
	}

	// TODO: Handle reference counting

	return pTexture;
}

static TextureSystem* s_TextureSystem = nullptr;
TextureSystem* gpTextureSystem = s_TextureSystem;
