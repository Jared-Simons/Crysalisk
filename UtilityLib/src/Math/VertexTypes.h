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
	Vertex3D(Vector2D&& inPos, Vector3D&& inColor, Vector2D&& inTexCoord)
		: Pos{ inPos }, Color{ inColor }, TexCoord{ inTexCoord } {}

	Vector2D Pos;
	Vector3D Color;
	Vector2D TexCoord;
};
