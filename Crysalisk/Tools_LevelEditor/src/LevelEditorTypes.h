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

enum eEditorToolMode
{
	EDITOR_TOOL_MODE_NONE = 0x0,

	// Gizmo active and manipulating selected geometry/open to selecting other geometry.
	EDITOR_TOOL_MODE_SELECTION = 0x1,
	// Add a model on mouse click and place it directly where the mouse is.
	EDITOR_TOOL_MODE_ADD_MODEL = 0x2,
};

struct MapEditorDocument_t
{
	Utl::CVector<MeshHandle_t>	meshes{};
};
