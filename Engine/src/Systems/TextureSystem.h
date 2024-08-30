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

struct TextureSystemConfiguration
{
	size_t maxTextureCount;
};

class TextureSystem
{
public:
	bool	Initialize(void* pSystemConfig);
	void	Shutdown();

	// TODO: Texture destruction!
	bool	LoadTexture(const char* pTexturePath);
	void	RegisterTexture(Texture* pTexture);
private:
	CHashmap<const char*, Texture*>	m_RegisteredTextures;
	size_t							m_NumRegisteredTextures;
};

ENGINE_API extern TextureSystem* gpTextureSystem;
