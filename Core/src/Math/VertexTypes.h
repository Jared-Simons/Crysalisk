//=============================================================================
// Author: Jared Simons
// Date Created: 7/3/2024
// 
// Purpose: Adds Vertex Types
//=============================================================================

#pragma once

#include <Defines.h>

#include "VectorTypes.h"

class Vertex3D
{
public:
	Vertex3D(Vector3D&& inPos, Vector3D&& inColor, Vector2D&& intexCoord)
		: Pos{ inPos }, Color{ inColor }, TexCoord{ intexCoord } {}

	Vector3D Pos;
	Vector3D Color;
	Vector2D TexCoord;
	Vector3D Normal;
	Vector3D Tangent;
};
