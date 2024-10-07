//=============================================================================
// Author: Jared Simons
// Date Created: 8/28/2024
// 
// Purpose: Texture System
//=============================================================================
#pragma once

#include "EngineGlobals.h"
#include "Resources/ResourceTypes.h"

#include <Containers/CHashmap.h>

// Forward declarations
class Renderer;

struct TextureSystemConfiguration
{
	size_t maxTextureCount;
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
	CHashmap<const char*, Texture>	m_RegisteredTextures;
	size_t							m_NumRegisteredTextures;

	Renderer*						m_RenderSystem;
};

ENGINE_API extern TextureSystem* gpTextureSystem;
