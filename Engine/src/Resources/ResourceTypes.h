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
#include <Renderer/RendererTypes.h>
#include <Strings/String_Utils.h>

using TextureHandle_t = uint32_t;
using TextureFlags = uint8_t;

enum TextureFlags_t
{
	TEXTURE_FLAG_DEFAULT_TEXTURE = 0x01,
	TEXTURE_FLAG_IS_TRANSPARENT = 0x02,
};

enum class eTextureType
{
	UNDEFINED = -1,
	DIFFUSE,
	SPECULAR,
	NORMAL,

	MAX_TEXTURE_TYPES,
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
	inline TextureFlags GetFlags() const { return textureFlags; }

private:
	int32_t width;
	int32_t height;
	int32_t numChannels; // TODO: Image format
	char* pszTextureName{ nullptr };
	TextureFlags textureFlags{ 0 };
	void* m_pInternalData{ nullptr };

	friend class CTextureSystem;
};

enum ShaderTopologyType
{
	UNDEFINED = 0,
	LINE_LIST = 1,
	TRIANGLE_LIST = 2,
};

using FontHandle_t = uint32_t;

struct FontAtlas_t
{
	size_t	Width;
	size_t	Height;
};

class Font
{
public:
	FontAtlas_t m_Atlas{};
	void* m_pInternalData{ nullptr };
	TextureHandle_t	m_hFontTexture{ kInvalidID_U32 };
};

using ShaderHandle_t = uint32_t;

struct ShaderConfig_t
{
	const char* pszShaderName{ nullptr };
	int			numShaderStages{ 0 };
	Utl::CVector<char*> pszShaderStageFileNames{};
	unsigned int eTopologyTypes{ UNDEFINED };
	Utl::CVector<VertexAttribute_t> vertexAttributes{};
	size_t		vertexStride{};
	Utl::CVector<ShaderUniformConfig_t> uniformConfigs{};
	RendererView	m_RendererView{};
};

class Shader
{
public:
	const char*		m_szShaderName{ nullptr };
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
};

using MaterialHandle_t = uint32_t;

struct MaterialConfig_t
{
	char* pszMaterialName{ nullptr };
	char* pszShaderName{ nullptr };
	int   iShininess{ 1 };
	char* pszDiffuseTextureName{ nullptr };
	char* pszSpecularTextureName{ nullptr };
	char* pszNormalTextureName{ nullptr };

	Vector3D vecDiffuseColor{};

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

		iShininess = pOther.iShininess;

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
	const char* m_szMaterialName{ nullptr };
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
	ShaderHandle_t	m_hShaderHandle{ kInvalidID_U32 };
	unsigned int	m_iShininess{ kInvalidID_U32 };
	bool			m_bIsTransparent{ false };
	TextureHandle_t	m_hDiffuseTextureHandle{ kInvalidID_U32 };
	TextureHandle_t	m_hSpecularTextureHandle{ kInvalidID_U32 };
	TextureHandle_t m_hNormalTextureHandle{ kInvalidID_U32 };
};

struct MaterialReference
{
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
	ShaderHandle_t	m_hShaderHandle{ kInvalidID_U32 };
	TextureHandle_t m_hDiffuseTextureHandle{ kInvalidID_U32 };
	bool			m_bIsTransparent{ false };
};

struct GeometryConfig2D_t
{
	Utl::CVector<Vertex2D> Vertices{};
	Utl::CVector<uint32_t> Indices{};
	MaterialReference MaterialReference{};
};

struct GeometryConfig_t
{
	Utl::CVector<Vertex3D> Vertices{};
	Utl::CVector<uint32_t> Indices{};
	MaterialReference MaterialReference{};
};

// TODO: This is temporary and should NOT stick around
struct GeometryInstance_t
{
	size_t vertexCount{ kInvalidID_U64 };
	size_t vertexDataOffset{ kInvalidID_U64 };
	size_t vertexDataStride{ kInvalidID_U64 };
	size_t indexDataSize{ kInvalidID_U64 };
	size_t indexDataOffset{ kInvalidID_U64 };
	size_t indexDataStride{ kInvalidID_U64 };

	MaterialReference hMaterial;
};

struct Mesh
{
	Utl::CVector<GeometryInstance_t> m_Geometries{};
};