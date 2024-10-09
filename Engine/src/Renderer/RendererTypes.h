//=============================================================================
// Author: Jared Simons
// Date Created: 10/8/2024
// 
// Purpose: Generic types used by the render system.
//=============================================================================
#pragma once

#include "Defines.h"
#include <Math/MatrixTypes.h>

struct GlobalUniformObject
{
	Matrix4x4 view;
	Matrix4x4 projection;
};

struct InstanceUniformObject
{
	Vector4D color;
	Vector4D reserved1; // for padding
	Vector4D reserved2; // for padding
	Vector4D reserved3; // for padding
};