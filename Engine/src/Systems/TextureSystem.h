//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The texture manager.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

struct TextureSystemConfig_t
{
	size_t	MaxTextures;
};

class ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual TextureHandle_t	LoadTexture(const char* pszTextureName) = 0;
	virtual void			ReleaseTexture(TextureHandle_t textureHandle) = 0;

	friend class Texture;
};

DLL_EXPORT ITextureSystem* g_pTextureSystem;