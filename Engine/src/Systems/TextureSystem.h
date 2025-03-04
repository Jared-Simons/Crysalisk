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
	size_t	MaxTextures{ 0 };
};

static const char* kszDefaultTextureName = "_DefaultDiffuse";
static const char* kszDefaultWhiteTextureName = "_DefaultWhite";

class ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName) = 0;
	virtual void				ReleaseTexture(TextureHandle_t hTexture, bool bForceRelease) = 0;

	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) = 0;

	virtual TextureHandle_t		GetDefaultWhiteTexture() = 0;
};

DLL_EXPORT ITextureSystem* g_pTextureSystem;