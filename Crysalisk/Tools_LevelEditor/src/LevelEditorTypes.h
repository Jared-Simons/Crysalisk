//=============================================================================
// Author: Jared Simons
// Date Created: 7/16/2022
// 
// Purpose: Level editor tool type declarations.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Containers/CVector.h>

struct Gizmo
{
	// LEFTOFF: Vertex color is needed.
};

struct MapEditorDocument_t
{
	Utl::CVector<MeshHandle_t>	meshes{};
};
