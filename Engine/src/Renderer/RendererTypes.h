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
	Matrix4x4	Reserved1; 
};

struct InstanceUniformObject_t
{
	Matrix4x4	Reserved0;
	Matrix4x4	Reserved1;
	Matrix4x4	Reserved2;
	Matrix4x4	Reserved3;
};