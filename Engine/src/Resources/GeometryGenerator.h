//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2025
// 
// Purpose: CSG Geometry Generation Tool
//=============================================================================
#pragma once

#include <Defines.h>
#include "ResourceTypes.h"

class ENGINE_API MeshGenerator
{
public:
	static GeometryConfig_t CreateDefaultQuadConfiguration(Vector3D vecOrigin, float flWidth, float flHeight);
	static GeometryConfig_t CreateDefaultCubeConfiguration(Vector3D vecOrigin, float flWidth, float flHeight, float flDepth);

	static void				GenerateTangentData(GeometryConfig_t& pConfig);
};

class IModelLoader
{
public:
	virtual Mesh CreateConfigFromFile(const char* pszFilePath) = 0;
};

extern IModelLoader* g_pOBJLoader;