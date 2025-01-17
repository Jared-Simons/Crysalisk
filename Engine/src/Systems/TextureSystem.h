//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The texture manager.
//=============================================================================
#pragma once

#include <Defines.h>

typedef uint32_t TextureHandle_t;

class Texture
{
public:

};

struct TextureSystemConfig_t
{
	size_t	MaxTextures;
};

class ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;
};

DLL_EXPORT ITextureSystem* g_pTextureSystem;