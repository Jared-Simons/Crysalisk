//=============================================================================
// Author: Jared Simons
// Date Created: 7/23/2026
// 
// Purpose: Editor gizmo.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Renderer/Camera.h>	// For Plane	(TODO: Move to some geometry utilities)

// Engine
#include <Resources/ResourceTypes.h>

struct Gizmo
{
	// The gizmo's origin in worldspace.
	Vector				origin{};

	GeometryInstance_t	geometry{};

	// Min/Max bounds for each axis.
	Vector				mins[3];
	Vector				maxs[3];

	u8					selectedAxis{ kInvalidID_U8 };

	Plane				interactionPlane{};

	// FIXME: Combine into flags.
	bool				geometryDirty{ false };
	bool				dragging{ false };
	Vector				lastDragPos{};
};

bool Gizmo_Create(class IRenderer* pRenderer, Gizmo* pOutGizmo);
void Gizmo_Destroy(class IRenderer* pRenderer, Gizmo* pGizmo);

void Gizmo_Update(Gizmo* pGizmo);

void Gizmo_BeginInteraction(struct Gizmo* pGizmo, struct Ray_t* pRay);
void Gizmo_EndInteraction(struct Gizmo* pGizmo);
void Gizmo_HandleInteraction(struct Gizmo* pGizmo, struct Camera_t* pCamera);

bool Gizmo_Raycast(struct Gizmo* pGizmo, struct Ray_t* pRay, u8* pOutHitAxis);