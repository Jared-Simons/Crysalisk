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
static const char* kszDefaultSpecularTextureName = "_DefaultSpecular";
static const char* kszDefaultNormalTextureName = "_DefaultNormal";

class ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName, eKnownTextureTypes textureType) = 0;
	virtual TextureHandle_t		WrapTextureInternal(const char* pszTextureName, size_t iTexWidth, size_t iTexHeight, byte* pPixelData) = 0;
	virtual void				ReleaseTexture(TextureHandle_t hTexture, bool bForceRelease) = 0;

	virtual int32_t				GetTextureWidth(TextureHandle_t hTexture) const = 0;
	virtual int32_t				GetTextureHeight(TextureHandle_t hTexture) const = 0;

	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) = 0;

	virtual TextureHandle_t		GetDefaultDiffuseTexture() = 0;
	virtual TextureHandle_t		GetDefaultWhiteTexture() = 0;
	virtual TextureHandle_t		GetDefaultSpecularTexture() = 0;
	virtual TextureHandle_t		GetDefaultNormalTexture() = 0;
};

DLL_EXPORT ITextureSystem* g_pTextureSystem;