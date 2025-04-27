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

enum class VertexType_t
{
	Undefined,
	Int32,
	Vector2,
	Vector3,
	Vector4,
	Matrix4x4,
	Texture,
};

struct VertexAttribute_t
{
	const char* szName;
	VertexType_t type;

	VertexAttribute_t()
		: szName{ nullptr }
		, type{ VertexType_t::Undefined } {}

	// TODO: String cleanup
};

enum class eShaderUniformUpdateFrequency
{
	GLOBAL,
	OBJECT,
	PER_DRAW
};

struct ShaderUniformConfig_t
{
	const char* szName;
	VertexType_t eType;
	eShaderUniformUpdateFrequency eFrequency;
	int iArraySize;
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