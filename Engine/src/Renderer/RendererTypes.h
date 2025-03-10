//=============================================================================
// Author: Jared Simons
// Date Created: 10/8/2024
// 
// Purpose: Generic types used by the render system.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>

//-------------------------------------------------------------------------
// Purpose: Frame-specific data to be uused in rendering operations.
//-------------------------------------------------------------------------
struct RenderPacket_t
{
};

struct GlobalUniformObject_t
{
	Matrix4x4	Projection;
	Matrix4x4	View;
	Vector3D	ViewPos;
	Vector4D	Reserved1; // NOTE: All of these exist so we can be aligned with Instance uniforms in the buffer. (My GPU needs these to be aligned to 64 bytes).
	Vector4D	Reserved2; 
	Vector3D	Reserved3;
	Vector2D	Reserved4;
};

struct InstanceUniformObject_t
{
	Vector4D	Reserved0;
	Vector4D	Reserved1;
	Vector4D	Reserved2;
	Vector4D	Reserved3;
};