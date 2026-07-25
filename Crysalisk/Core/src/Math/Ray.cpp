//=============================================================================
// Author: Jared Simons
// Date Created: 7/24/2026
// 
// Purpose: Ray-related utilities.
//=============================================================================

#include "Ray.h"

// Core
#include <Core/Assertions.h>
#include <Renderer/Camera.h>

Ray_t GetRayFromScreen(Platform::IPlatformWindow* pWindow, struct Camera_t* pCamera, i32 mouseXPos, i32 mouseYPos)
{
	CASSERTMSG(pWindow && pCamera, "GetRayFromScreen() - requires valid pointers to a window and camera!");

	Ray_t r{};

	u32 windowWidth{ 0 }, windowHeight{ 0 };
	pWindow->GetWindowSize(windowWidth, windowHeight);

	// mouseXPos and mouseYPos are the screen-space coordinates of the mouse cursor.
	// Bring these coordinates to NDC.
	Vector4D clipPos{};
	clipPos.x = (2.0f * mouseXPos) / windowWidth - 1.0f;
	clipPos.y = 1.0f - (2.0f * mouseYPos) / windowHeight;
	clipPos.z = -1.0f;
	clipPos.w = 1.0f;

	// Clip space -> Camera space
	// Get the inverse projection matrix from the window's viewport.
	Matrix4x4 inverseProj = pWindow->GetViewport().matProjection.Inverse();
	Vector4D cameraSpacePos = inverseProj * clipPos;
	cameraSpacePos.z = -1.0f;
	cameraSpacePos.w = 0.0f;

	// Camera space -> World space
	Vector rayDir = (pCamera->GetViewMatrix().Inverse() * cameraSpacePos).AsVector3D();
	rayDir.NormalizeInPlace();

	r.vecOrigin = pCamera->vecPosition;
	r.vecDirection = rayDir;
	r.flDistance = kFLT_MAX;
	r.bDidHit = false;

	return r;
}