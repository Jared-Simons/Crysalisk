//=============================================================================
// Author: Jared Simons
// Date Created: 8/28/2024
// 
// Purpose: Texture System
//=============================================================================
#pragma once

#include "Defines.h"
#include "Resources/ResourceTypes.h"

#include <Containers/CHashmap.h>

// Forward declarations
class Renderer;

struct TextureSystemConfiguration
{
	u64 maxTextureCount;
};

class TextureSystem
{
public:
	bool	Initialize(void* pSystemConfig, Renderer* pRenderSystem);
	void	Shutdown();

	// TODO: Texture destruction!
	bool	LoadTexture(const char* pTexturePath);
	void	UnloadTexture(Texture& pTexture);

	Texture*	AcquireTexture(const char* pTexturePath);
private:
	TextureSystemConfiguration		m_Configuration;

	CHashmap<const char*, Texture>	m_RegisteredTextures;
	u64								m_NumRegisteredTextures;

	Renderer*						m_RenderSystem;
};

ENGINE_API extern TextureSystem* gpTextureSystem;
