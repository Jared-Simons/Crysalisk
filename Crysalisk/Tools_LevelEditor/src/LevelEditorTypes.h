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
	GeometryInstance_t geometry{};
};

struct MapEditorDocument_t
{
	Utl::CVector<MeshHandle_t>	meshes{};
};
