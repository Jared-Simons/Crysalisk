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

#ifdef LEVEL_EDITOR_DLL
	#define LEVEL_EDITOR_API extern "C" __declspec(dllexport)
#else
	#define LEVEL_EDITOR_API extern "C" __declspec(dllimport)
#endif

struct Gizmo
{
	// The gizmo's origin in worldspace.
	Vector				origin{};

	GeometryInstance_t	geometry{};
};

struct MapEditorDocument_t
{
	Utl::CVector<MeshHandle_t>	meshes{};
};
