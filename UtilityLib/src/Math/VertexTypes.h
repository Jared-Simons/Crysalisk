//=============================================================================
// Author: Jared Simons
// Date Created: 7/3/2024
// 
// Purpose: Adds Vertex Types
//=============================================================================

#pragma once

#include "VectorTypes.h"

class Vertex3D
{
public:

	Vertex3D(Vector2D&& inPos, Vector3D&& inColor)
		: Pos{ inPos }, Color{ inColor } {}

	Vector2D Pos;
	Vector3D Color;
};
