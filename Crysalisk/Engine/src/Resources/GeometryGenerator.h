//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2025
// 
// Purpose: CSG Geometry Generation Tool
//=============================================================================
#pragma once

#include <Defines.h>
#include <Math/MathLib.h>
#include "ResourceTypes.h"

class ENGINE_API MeshGenerator
{
public:
	// Creates a simple quad configuration of flWidth and flHeight that can be used for rendering.
	static GeometryConfig_t CreateDefaultQuadConfiguration(Vector3D vecOrigin, f32 flWidth, f32 flHeight);
	// Creates a simple cube configuration of flWidth, flHeight, and flDepth that can be used for rendering.
	static GeometryConfig_t CreateDefaultCubeConfiguration(Vector3D vecOrigin, f32 flWidth, f32 flHeight, f32 flDepth);

	// Given a geometry configuration, generates tangent data necessary for lighting calculations.
	static void				GenerateTangentData(GeometryConfig_t& pAppConfig);
};