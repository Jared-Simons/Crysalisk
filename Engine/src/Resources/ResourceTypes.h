//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Containers/CVector.h>
#include <Math/VectorTypes.h>
#include <Math/VertexTypes.h>

using TextureHandle_t = uint32_t;
using TextureFlags = uint8_t;

enum TextureFlags_t
{
	TEXTURE_FLAG_DEFAULT_TEXTURE = 0x01
};

class Texture
{
public:
	inline void	SetInternalTextureData(void* pInternalData) { m_pInternalData = pInternalData; }
	inline void*	GetInternalData() const { return m_pInternalData; } // TODO: I don't like this :(
	inline int32_t	GetWidth() const { return width; }
	inline int32_t GetHeight() const { return height; }
	inline int32_t GetNumChannels() const { return numChannels; }
	inline const char* GetName() const { return pszTextureName; }

private:
	int32_t width;
	int32_t height;
	int32_t numChannels; // TODO: Image format
	char* pszTextureName{ nullptr };
	TextureFlags textureFlags{ 0 };
	void* m_pInternalData{ nullptr };

	friend class CTextureSystem;
};

struct GeometryConfig_t
{
	Utl::CVector<Vertex3D> Vertices{};
	Utl::CVector<uint32_t> Indices{};
	const char* pTextureName{};
};

// TODO: This is temporary and should NOT stick around
struct GeometryInstance_t
{
	int	vertexCount{};
	int vertexDataOffset{};
	int indexDataSize{};
	int indexDataOffset{};

	TextureHandle_t pTexture{};
};