//=============================================================================
// Author: Jared Simons
// Date Created: 5/21/2026
// 
// Purpose: Debug Geometry Generation Tool
//=============================================================================
#pragma once

#include <Defines.h>
#include <Math/MathLib.h>
#include "Resources/ResourceTypes.h"

namespace DebugRender
{
	//-------------------------------------------------------------------------
	// Purpose: An instance of debug geometry, contains all of the
	//			data/resources necessary to render debug geometry.
	//-------------------------------------------------------------------------
	struct DebugGeometry_t
	{
		uint32_t			iInstanceID{ kInvalidID_U32 };
		Vector4D			color{};
		GeometryInstance_t	geometryInstance{};
		float				flTimeToRemove{ 0.0f };
	};

	DebugGeometry_t	AddPoint(Vector ptOrigin, const float flRadius, Vector4D color, float flDuration);
	DebugGeometry_t AddLine(Vector ptStartPos, Vector ptEndPos, Vector4D color, float flDuration);
	DebugGeometry_t AddBBox(GeometryInstance_t& pGeometry, Vector4D color, float flDuration);
}; // namespace DebugRender
