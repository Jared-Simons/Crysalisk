//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Containers/CVector.h>
#include <Math/VectorTypes.h>
#include <Math/VertexTypes.h>

struct GeometryConfig_t
{
	Utl::CVector<Vertex3D> Vertices{};
	Utl::CVector<uint32_t> Indices{};
};