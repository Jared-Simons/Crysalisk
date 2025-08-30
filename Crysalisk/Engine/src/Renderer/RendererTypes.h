//=============================================================================
// Author: Jared Simons
// Date Created: 10/8/2024
// 
// Purpose: Generic types used by the render system.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Containers/CFreeList.h>
#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Purpose: Frame-specific data to be used in rendering operations.
//-------------------------------------------------------------------------
struct RenderPacket_t
{
	// The time elapsed between frames.
	double DeltaTime;
};

//-------------------------------------------------------------------------
// Purpose: Stats collected about renderer metrics.
//-------------------------------------------------------------------------
struct RenderMetrics_t
{
	// The frametime of the previous frame.
	double	FrameTime{ 0.0 };
	// The number of individual draw calls made this frame.
	int		NumDrawCalls{ 0 };
};

//-------------------------------------------------------------------------
// Purpose: Per-frame data used in shaders.
//-------------------------------------------------------------------------
struct GlobalUniformObject_t
{
	// Projection matrix.
	Matrix4x4	Projection;
	// View matrix.
	Matrix4x4	View;
	// The current view position.
	Vector4D	ViewPos;
	// The position of the directional light. TODO: We don't need this at all.
	Vector3D	DirLightPos;
	// The current render visualization mode.
	int			RenderMode;
	Vector3D	Reserved1; // NOTE: All of these exist so we can be aligned with Instance uniforms in the buffer. (My GPU needs these to be aligned to 64 bytes).
	Vector3D	Reserved2; 
	Vector2D	Reserved3;
};

//-------------------------------------------------------------------------
// Purpose: Per-instance data used in shaders.
//-------------------------------------------------------------------------
struct MaterialInstanceData_t
{
	// The shininess factor of a material.
	float		Shininess;
	// The diffuse tint of a material.
	Vector3D	vecDiffuseColor;
	Vector4D	Reserved0;
	Vector4D	Reserved2;
	Vector4D	Reserved3;
};

//-------------------------------------------------------------------------
// Purpose: Types that can be used in shaders.
//-------------------------------------------------------------------------
enum class ShaderAttribute_t
{
	Undefined,
	Int32,
	Float32,
	Vector2,
	Vector3,
	Vector4,
	Matrix4x4,
	GenericStructure,
	Sampler,
};

//-------------------------------------------------------------------------
// Purpose: A vertex attribute used in a shader.
//-------------------------------------------------------------------------
struct VertexAttribute_t
{
	// The name of the attribute.
	char*				szName{ nullptr };
	// The attribute's type.
	ShaderAttribute_t	type{ ShaderAttribute_t::Undefined };
	// The length of the attribute's data.
	uint32_t			stride{ 0 };

	VertexAttribute_t()
		: szName{ nullptr }
		, type{ ShaderAttribute_t::Undefined } 
		, stride{ 0 } {}

	VertexAttribute_t(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;
	}

	VertexAttribute_t& operator=(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;

		return *this;
	}

	~VertexAttribute_t()
	{
		if (szName)
			Utl::StringFree(szName);
	}
};

//-------------------------------------------------------------------------
// Purpose: The group a shader's uniform belongs to.
//-------------------------------------------------------------------------
enum class eShaderUniformGroup
{
	// These uniforms get updated once per-frame, typically global data like translation matrices.
	PER_FRAME,
	// These uniforms get updated once per-instance, per-frame, typically for material-specific data.
	PER_MATERIAL,
	// The uniforms get updated once per-draw, typically used for the model matrix.
	PER_DRAW
};

//-------------------------------------------------------------------------
// Purpose: The configuration data for a shader uniform.
//-------------------------------------------------------------------------
struct ShaderUniformConfig_t
{
	// The name of the uniform.
	char*							szName;
	// The uniform's type.
	ShaderAttribute_t				eType;
	// The uniform's update group.
	eShaderUniformGroup				eFrequency;
	// The length of a single element in bytes (does not account for a whole array).
	uint32_t						iStride;	
	// The size of the uniform's array.
	int								iArraySize;

	ShaderUniformConfig_t()
		: szName{ nullptr }
		, eType{ ShaderAttribute_t::Undefined }
		, eFrequency{ eShaderUniformGroup::PER_MATERIAL }
		, iStride{ 0 }
		, iArraySize{ 1 } { }

	ShaderUniformConfig_t(const ShaderUniformConfig_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		else
			szName = nullptr;
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;
	}

	ShaderUniformConfig_t& operator=(const ShaderUniformConfig_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;

		return *this;
	}

	~ShaderUniformConfig_t()
	{
		if (szName)
			Utl::StringFree(szName);
	}
};

// TODO: We shouldn't need to keep these around.
enum class eKnownRenderpassType
{
	WORLD,
	UI
};

//-------------------------------------------------------------------------
// Purpose: The different types of renderbuffers.
//-------------------------------------------------------------------------
enum class eRenderBufferUsage
{
	// This buffer is used to store vertex data for drawing.
	VERTEX,
	// This buffer is used to store index data for drawing.
	INDEX,
	// This buffer is used to store uniform data for use in shaders.
	UNIFORM,
	// This buffer is used for other storage purposes.
	STORAGE
};

//-------------------------------------------------------------------------
// Purpose: A generic buffer used for storing data.
//-------------------------------------------------------------------------
class RenderBuffer
{
public:
	// A pointer to the internal data of the buffer.
	void* m_pInternalData{ nullptr };
	// TODO: Store the type off here as well?
};

//-------------------------------------------------------------------------
// Purpose: A generic renderer view (internally a renderpass).
//-------------------------------------------------------------------------
class RendererView
{
public:
	// Indicates if the renderer view should clear it's render target before rendering.
	bool	m_bShouldClear{ false };
	// A pointer to the internal data of the buffer.
	void*	m_pInternalData{ nullptr };
};