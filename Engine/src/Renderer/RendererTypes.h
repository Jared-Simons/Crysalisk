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
	double DeltaTime;
};

struct GlobalUniformObject_t
{
	Matrix4x4	Projection;
	Matrix4x4	View;
	Vector4D	ViewPos;
	Vector3D	DirLightPos;
	int			RenderMode;
	Vector3D	Reserved1; // NOTE: All of these exist so we can be aligned with Instance uniforms in the buffer. (My GPU needs these to be aligned to 64 bytes).
	Vector3D	Reserved2; 
	Vector2D	Reserved3;
};

struct MaterialInstanceData_t
{
	int			Shininess;
	Vector3D	vecDiffuseColor;
	Vector4D	Reserved0;
	Vector4D	Reserved2;
	Vector4D	Reserved3;
};

enum class ShaderAttribute_t
{
	Undefined,
	Int32,
	Float32,
	Vector2,
	Vector3,
	Vector4,
	Matrix4x4,
	Texture,
};

struct VertexAttribute_t
{
	char*			szName;
	ShaderAttribute_t	type;
	size_t			stride;

	VertexAttribute_t()
		: szName{ nullptr }
		, type{ ShaderAttribute_t::Undefined } 
		, stride{ 0 } {}

	VertexAttribute_t(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;
	}

	VertexAttribute_t& operator=(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;

		return *this;
	}

	~VertexAttribute_t()
	{
		if (szName)
			StringFree(szName);
	}
};

enum class eShaderUniformGroup
{
	PER_FRAME,
	PER_MATERIAL,
	PER_DRAW
};

struct ShaderUniformConfig_t
{
	char*							szName;
	ShaderAttribute_t				eType;
	eShaderUniformGroup				eFrequency;
	size_t							iStride;	// The length of a single element in bytes (does not account for a whole array).
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
			szName = StringDuplicate(pOther.szName);
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;
	}

	ShaderUniformConfig_t& operator=(const ShaderUniformConfig_t& pOther)
	{
		if (pOther.szName)
			szName = StringDuplicate(pOther.szName);
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;

		return *this;
	}

	~ShaderUniformConfig_t()
	{
		if (szName)
			StringFree(szName);
	}
};

enum class eKnownRenderpassType
{
	WORLD,
	UI
};

enum class eRenderBufferUsage
{
	VERTEX,
	INDEX,
	UNIFORM,
	STORAGE
};

class RenderBuffer
{
public:
	void* m_pInternalData{ nullptr };
};

class RendererView
{
public:
	bool	m_bShouldClear{ false };
	void*	m_pInternalData{ nullptr };
};