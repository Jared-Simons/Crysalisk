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
#include <Strings/String_Utils.h>

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

using MaterialHandle_t = uint32_t;

struct MaterialConfig_t
{
	char* pszMaterialName{ nullptr };
	char* pszShaderName{ nullptr };
	char* pszDiffuseTextureName{ nullptr };
	char* pszSpecularTextureName{ nullptr };
	char* pszNormalTextureName{ nullptr };

	MaterialConfig_t() {}

	MaterialConfig_t(MaterialConfig_t&& pOther) noexcept
	{
		if (pOther.pszMaterialName != nullptr)
		{
			pszMaterialName = StringDuplicate(pOther.pszMaterialName);
		}

		if (pOther.pszShaderName != nullptr)
		{
			pszShaderName = StringDuplicate(pOther.pszShaderName);
		}

		if (pOther.pszDiffuseTextureName != nullptr)
		{
			pszDiffuseTextureName = StringDuplicate(pOther.pszDiffuseTextureName);
		}

		if (pOther.pszSpecularTextureName != nullptr)
		{
			pszSpecularTextureName = StringDuplicate(pOther.pszSpecularTextureName);
		}

		if (pOther.pszNormalTextureName != nullptr)
		{
			pszNormalTextureName = StringDuplicate(pOther.pszNormalTextureName);
		}
	}

	~MaterialConfig_t()
	{
		if (pszMaterialName != nullptr)
		{
			StringFree(pszMaterialName);
			pszMaterialName = nullptr;
		}

		if (pszShaderName != nullptr)
		{
			StringFree(pszShaderName);
			pszShaderName = nullptr;
		}

		if (pszDiffuseTextureName != nullptr)
		{
			StringFree(pszDiffuseTextureName);
			pszDiffuseTextureName = nullptr;
		}

		if (pszSpecularTextureName != nullptr)
		{
			StringFree(pszSpecularTextureName);
			pszSpecularTextureName = nullptr;
		}

		if (pszNormalTextureName != nullptr)
		{
			StringFree(pszNormalTextureName);
			pszNormalTextureName = nullptr;
		}
	}
};

class Material
{
public:
	// TODO: Store shader handle here.
	const char*		m_szMaterialName{ nullptr };
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
	TextureHandle_t	m_hDiffuseTextureHandle{ kInvalidID_U32 };
	TextureHandle_t	m_hSpecularTextureHandle{ kInvalidID_U32 };
	TextureHandle_t m_hNormalTextureHandle{ kInvalidID_U32 };
};

struct MaterialReference
{
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
	TextureHandle_t m_hDiffuseTextureHandle{ kInvalidID_U32 };
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

	MaterialReference hMaterial;
};