//=============================================================================
// Author: Jared Simons
// Date Created: 5/25/2025
// 
// Purpose: Camera types, typically used for rendering.
//=============================================================================
#pragma once

#include "Core/Input.h"
#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>
#include "Renderer/RendererTypes.h"

//-------------------------------------------------------------------------
// A general purpose camera.
//-------------------------------------------------------------------------
struct Camera_t
{
	// The camera's position in world space.
	Vector3D_OLD vecPosition{ 0, 0, 3 };
	// The camera's current forward vector.
	Vector3D_OLD vecFront{ 0, 0, -1 };
	// The camera's current right vector.
	Vector3D_OLD vecRight{ 1, 0, 0 };
	// The camera's current up vector.
	Vector3D_OLD vecUp{ 0, 1, 0 };
	// The absolute up vector in world space.
	Vector3D_OLD vecWorldUp{ 0, 1, 0 };

	float	Yaw{ -90.0f };
	float	Pitch{ 0.0f };
	float	MoveSpeed{ 6.0f };
	float	TurnSpeed{ 0.5f };

	float	flFOV{ 75.0f };
	float	flZNear{ 0.0f };
	float	flZFar{ 10000.0f };
	float	flAspectRatio{ 16 / 9.0f };

	// Returns the camera's view matrix.
	inline Matrix4x4_OLD GetViewMatrix() { return Matrix4x4_OLD::LookAt(vecPosition, vecPosition + vecFront, vecUp); }

	// Recalculate's the camera's internal vectors, this is typically needed when the camera's rotation changes.
	void RecalculateCameraVectors();

	// A simple update function.
	void UpdateCameraData(double deltaTime);

	// Creates and returns a frustum based on the camera's internal state.
	Frustum CreateFrustumFromCamera();
};