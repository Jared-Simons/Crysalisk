//=============================================================================
// Author: Jared Simons
// Date Created: 7/3/2024
// 
// Purpose: Adds Vertex Types
//=============================================================================

#pragma once

#include <Defines.h>

#include "VectorTypes.h"

class Vertex2D
{
public:
	Vector2D Pos;
	Vector4D Color;
	Vector2D TexCoord;
};

class Vertex3D
{
public:
	Vertex3D()
		: Pos{}, TexCoord{}, Normal{}, Tangent{} {}

	Vertex3D(Vector3D&& inPos, Vector3D&& inColor, Vector2D&& intexCoord)
		: Pos{ inPos }, TexCoord{ intexCoord } {}

	bool operator==(const Vertex3D& pOther)
	{
		return ((Pos == pOther.Pos) &&
				(TexCoord == pOther.TexCoord) &&
				(Normal == pOther.Normal) /* &&
				(Tangent == pOther.Tangent)*/);		// NOTE: We don't want to consider tangent data in the equality of a vertex.
	}

	Vector3D Pos;
	//Vector3D Color;
	Vector2D TexCoord;
	Vector3D Normal;
	Vector3D Tangent;
};
