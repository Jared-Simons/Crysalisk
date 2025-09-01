//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Containers/CHashMap.h>
#include <Containers/CVector.h>
#include <Math/VectorTypes.h>
#include <Math/VertexTypes.h>
#include <Renderer/RendererTypes.h>
#include <Strings/String_Utils.h>

// A unique identifier for a texture.
using TextureHandle_t = uint32_t;
using TextureFlags = uint8_t;

// Bit-flags for specific texture attributes.
enum TextureFlags_t
{
	// Is this texture a default texture?
	TEXTURE_FLAG_DEFAULT_TEXTURE = 0x01,
	// Is this texture transparent in any way?
	TEXTURE_FLAG_IS_TRANSPARENT = 0x02,
};

// Different 'known' texture types.
enum class eTextureType
{
	UNDEFINED = -1,
	DIFFUSE,
	SPECULAR,
	NORMAL,

	MAX_TEXTURE_TYPES,
};

// The internal representation of a texture.
// This structure should not be used by anything except the texture system and internal renderer.
// Everything else should maintain references to textures via TextureHandle_t.
struct Texture
{
	// The width of the texture.
	int32_t			width;
	// The height of the texture.
	int32_t			height;
	// The number of channels this texture contains.
	int32_t			numChannels; // TODO: Image format
	// The name of this texture.
	char*			pszTextureName{ nullptr };
	// Texture flags.
	TextureFlags	textureFlags{ 0 };
	// The internal handle for the texture.
	uint32_t		internalHandle{ kInvalidID_U32 };
};

// A unique identifier for a font.
using FontHandle_t = uint32_t;

// The internal representation of a font.
struct Font
{
public:
	// The internal shader instance ID used for rendering this font.
	uint32_t		m_InternalInstanceID{ kInvalidID_U32 };
	// The font atlas texture map.
	TextureHandle_t	m_hFontTexture{ kInvalidID_U32 };
	// The internal data, typically font/glyph data used for rendering.
	void*			m_pInternalFontAtlasData{ nullptr };
};

// A unique identifier for a shader.
using ShaderHandle_t = uint32_t;

// Different shader stage/module types.
enum class eShaderStage
{
	// Undefined stage, typically an error.
	UNDEFINED,
	// This module is a vertex shader.
	VERTEX,
	// This module is a fragment shader.
	FRAGMENT,
	// This module is a geometry shader.
	GEOMETRY,
	// This module is a compute shader.
	COMPUTE,
};

// The type of topology rendering this shader will support.
enum eShaderTopologyType
{
	// Undefined type, typically an error.
	SHADER_TOPOLOGY_TYPE_UNDEFINED,
	// Renders lines.
	SHADER_TOPOLOGY_TYPE_LINE_LIST,
	// Renders triangle data.
	SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
};

// Represents the configuration for an individual shader module.
struct ShaderModuleConfig_t
{
	// The type of shader module this config contains (e.g. Vertex, Fragment, etc.).
	eShaderStage		eStage{ eShaderStage::UNDEFINED };
	// The size in bytes of this module's bytecode.
	size_t				iCodeSize{ 0 };
	// The module's bytecode.
	Utl::CVector<byte>	byteCode{};
	// The shader module's name, only really used for debugging.
	char*				pszModuleName{ nullptr };

	~ShaderModuleConfig_t()
	{
		if (pszModuleName)
			Utl::StringFree(pszModuleName);
		pszModuleName = nullptr;
		byteCode.Clear();
	}
};

// Represents the configuration for a shader program.
struct ShaderConfig_t
{
	// The name of the shader.
	const char*							pszShaderName{ nullptr };
	// An array of shader module configs.
	Utl::CVector<ShaderModuleConfig_t>	moduleConfigs{};
	// The types of topology rendering this shader will support.
	unsigned int						eTopologyTypes{ SHADER_TOPOLOGY_TYPE_UNDEFINED };
	// Indicates if this shader will support depth-testing.
	bool								bDepthTestingEnabled{ false };
	// Indicates if this shader will support color-blending.
	bool								bColorBlendingEnabled{ false };
	// An array of vertex attribute configs.
	Utl::CVector<VertexAttribute_t>		vertexAttributes{};
	// The stride of a vertex used by this shader.
	uint32_t							vertexStride{};
	// An array of uniform configs.
	Utl::CVector<ShaderUniformConfig_t> uniformConfigs{};
	// The length of this shader's per-frame data buffer.
	uint32_t							perFrameUBOStride{ 0 };
	// The length of this shader's per-instance data buffer.
	uint32_t							perInstanceUBOStride{ 0 };
	// The length of this shader's per-draw data buffer.
	uint32_t							perDrawUBOStride{ 0 };
	// A pointer to the renderer view used by this shader.
	RendererView						m_pRendererView{};
};

// Represents a shader uniform.
struct ShaderUniform_t
{
	// The offset into the data buffer that this uniform's data lives.
	uint32_t			Offset{ 0 };
	// The length in bytes of this uniform's data.
	uint32_t			Stride{ 0 };
	// The type of data this uniform uses.
	ShaderAttribute_t	eType{ ShaderAttribute_t::Undefined };
	// The group/update frequency of this uniform (e.g. per-frame, per-instance, per-draw).
	eShaderUniformGroup	eGroup{ eShaderUniformGroup::PER_FRAME };
};

// The internal representation of a compiled shader program.
class Shader
{
public:
	// This shader's name.
	const char*					m_pszShaderName{ nullptr };
	// This shader's internal handle (should be the same for this shader's ShaderHandle_t).
	unsigned int				m_iInternalHandle{ kInvalidID_U32 };
	// The current renderer frame (updated on shader bind).
	size_t						m_iRendererFrame{ kInvalidID_U64 };
	// A hashmap to store off indices for uniform indices.
	Utl::CStringHashMap<int>	m_UniformLookup{};
	// The number of uniforms in this shader.
	size_t						m_iUniformCount{ 0 };
	// An array of uniforms used by this shader.
	ShaderUniform_t*			m_pUniforms{ nullptr };

	// The length of this shader's per-frame data buffer.
	size_t						m_iPerFrameUBOSize{ 0 };
	// The length of this shader's per-instance data buffer.
	size_t						m_iPerInstanceUBOSize{ 0 };
	// The alignment requirement for this shader's uniform buffers.
	size_t						m_iUniformBufferAlignmentRequirement{ 0 };
};

// A unique identifier for a material.
using MaterialHandle_t = uint32_t;

// The internal representation of a material.
class Material
{
public:
	// This material's name.
	const char*		m_pszMaterialName{ nullptr };
	// The internal handle of this material.
	unsigned int	m_iInternalHandle{ kInvalidID_U32 };
	// The shader used to render this material.
	ShaderHandle_t	m_hShaderHandle{ kInvalidID_U32 };
	// The shininess exponent used to control the concentration of specular highlights.
	unsigned int	m_iShininess{ kInvalidID_U32 };
	// Indicates if this material is transparent.
	bool			m_bIsTransparent{ false };
	// This material's diffuse texture.
	TextureHandle_t	m_hDiffuseTextureHandle{ kInvalidID_U32 };
	// This material's specular texture.
	TextureHandle_t	m_hSpecularTextureHandle{ kInvalidID_U32 };
	// This material's normal texture.
	TextureHandle_t m_hNormalTextureHandle{ kInvalidID_U32 };
};

// Represents the configuration of a piece of 2D geometry (typically used for rendering UI).
struct GeometryConfig2D_t
{
	// An array of vertices for this geometry.
	Utl::CVector<Vertex2D> Vertices{};
	// An array of indices for this geomtry.
	Utl::CVector<uint32_t> Indices{};
};

// Represents the configuration of a piece of 3D geometry.
struct GeometryConfig_t
{
	// An array of vertices for this geometry.
	Utl::CVector<Vertex3D> Vertices{};
	// An array of indices for this geomtry.
	Utl::CVector<uint32_t> Indices{};

	// The name of this piece of geometry - typically used for debugging purposes.
	char* pszDebugName{ nullptr };

	// Release temporary string data.
	~GeometryConfig_t()
	{
		if (pszDebugName)
			Utl::StringFree(pszDebugName);

		pszDebugName = nullptr;
	}
};

// Represents an instance of geometry that has been uploaded and is ready for rendering.
struct GeometryInstance_t
{
	// The number of vertices in this mesh.
	size_t vertexCount{ kInvalidID_U64 };
	// The offset into the relevant vertex buffer that this mesh's vertex data begins.
	size_t vertexDataOffset{ kInvalidID_U64 };
	// The length in bytes of a single vertex used by this mesh.
	size_t vertexDataStride{ kInvalidID_U64 };
	// The number of indices in this mesh.
	size_t indexCount{ kInvalidID_U64 };
	// The offset into the relevant index buffer that this mesh's index data begins.
	size_t indexDataOffset{ kInvalidID_U64 };
	// The length in bytes of a single index used by this mesh.
	size_t indexDataStride{ kInvalidID_U64 };

	// The material used in rendering this mesh.
	MaterialHandle_t	hMaterial;
	// Indicates if the material used by this mesh is transparent.
	bool				bIsTransparent;
};

// Represents a mesh/model, made up of numerous submeshes.
struct Mesh
{
	// An array of submeshes that make up this mesh.
	Utl::CVector<GeometryInstance_t>	m_Geometries{};
	// Origin of this mesh in world space.
	Vector3D							m_vecAbsOrigin{};
};