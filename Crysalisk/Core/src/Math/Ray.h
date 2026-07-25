//=============================================================================
// Author: Jared Simons
// Date Created: 7/24/2026
// 
// Purpose: Ray-related utilities.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include "MathLib.h"
#include <Platform/Platform.h>

//-------------------------------------------------------------------------
// Represents a ray which can collide with objects in a scene.
//-------------------------------------------------------------------------
struct Ray_t
{
	// The starting point of the ray.
	Vector	vecOrigin{};
	// The direction the ray travels.
	Vector	vecDirection{};
	// The endpoint of the ray.
	Vector	vecEnd{};
	// Indicates if the ray hit anything.
	bool	bDidHit{ false };
	// The distance the ray travelled from its origin along it's direction vector.
	f32		flDistance{ 0.0f };
	// A pointer to the geometry this ray hit, if any.
	struct GeometryInstance_t* pHitGeometry{ nullptr };
};

// Sets up and returns a ray that can be cast from the camera's position with direction based on the mouse position.
// Does not actually perform the raycast.
CORE_API Ray_t GetRayFromScreen(Platform::IPlatformWindow* pWindow, struct Camera_t* pCamera, i32 mouseXPos, i32 mouseYPos);