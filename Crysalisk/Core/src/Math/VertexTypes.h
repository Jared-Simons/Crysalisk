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
	Vector2D_OLD Pos;
	Vector4D Color;
	Vector2D_OLD TexCoord;
};

class Vertex3D
{
public:
	Vertex3D()
		: Pos{}, TexCoord{}, Normal{}, Tangent{} {}

	Vertex3D(Vector3D_OLD&& inPos, Vector3D_OLD&& inColor, Vector2D_OLD&& intexCoord)
		: Pos{ inPos }, TexCoord{ intexCoord } {}

	bool operator==(const Vertex3D& pOther)
	{
		return ((Pos == pOther.Pos) &&
				(TexCoord == pOther.TexCoord) &&
				(Normal == pOther.Normal) &&
				(Tangent == pOther.Tangent));
	}

	Vector3D_OLD Pos;
	//Vector3D_OLD Color;
	Vector2D_OLD TexCoord;
	Vector3D_OLD Normal;
	Vector3D_OLD Tangent;
};
