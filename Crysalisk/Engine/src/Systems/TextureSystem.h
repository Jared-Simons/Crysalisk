//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The texture manager.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Resources/Loaders/ImageLoader.h>
#include "Resources/ResourceTypes.h"

struct TextureSystemConfig_t
{
	size_t	MaxTextures{ 0 };

	// A pointer to the render system.
	class IRenderer* pRenderer{ nullptr };
	// A pointer to the job system.
	class IJobSystem* pJobSystem{ nullptr };
};

static const char* kszDefaultTextureName = "_DefaultDiffuse";
static const char* kszDefaultWhiteTextureName = "_DefaultWhite";
static const char* kszDefaultSpecularTextureName = "_DefaultSpecular";
static const char* kszDefaultNormalTextureName = "_DefaultNormal";

class ITextureSystem
{
public:
	static void					AllocateTextureSystemState(ITextureSystem** ppSystemState);

	virtual bool				Initialize(TextureSystemConfig_t config) = 0;
	virtual void				Shutdown() = 0;

	virtual TextureHandle_t		AcquireTexture(const char* pszTextureName, bool bAutoRelease) = 0;
	virtual TextureHandle_t		AcquireTextureFromResource(ImageLoader::ImageResource_t& pResource, const char* pszTextureName, bool bAutoRelease) = 0;
	virtual void				ReleaseTexture(TextureHandle_t hTexture) = 0;

	virtual bool				IsTextureFlagSet(TextureHandle_t hTexture, TextureFlags_t flagsToCheck) const = 0;

	virtual TextureHandle_t		WrapTextureInternal(const char* pszTextureName, int iTexWidth, int iTexHeight, byte* pPixelData) = 0;

	virtual int32_t				GetTextureWidth(TextureHandle_t hTexture) const = 0;
	virtual int32_t				GetTextureHeight(TextureHandle_t hTexture) const = 0;

	virtual Texture*			GetTextureFromHandle(TextureHandle_t hTex) = 0;

	virtual TextureHandle_t		GetDefaultDiffuseTexture() = 0;
	virtual TextureHandle_t		GetDefaultWhiteTexture() = 0;
	virtual TextureHandle_t		GetDefaultSpecularTexture() = 0;
	virtual TextureHandle_t		GetDefaultNormalTexture() = 0;

	virtual void				SetTextureFlags(const TextureHandle_t hTexture, const TextureFlags_t& iFlags) = 0;
	virtual void				UnsetTextureFlags(const TextureHandle_t hTexture, const TextureFlags_t& iFlags) = 0;
};
