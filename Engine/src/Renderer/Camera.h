#pragma once

#include "Core/Input.h"
#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>

// TEMPORARY CAMERA DATA
// THIS SHOULD BE MOVED ELSEWHERE EVENTUALLY!
struct Camera_t
{
	Vector3D vecPosition{ 0, 0, 3 };
	Vector3D vecFront{ 0, 0, -1 };
	Vector3D vecRight{ 1, 0, 0 };
	Vector3D vecUp{ 0, 1, 0 };
	Vector3D vecWorldUp{ 0, 1, 0 };

	float	Yaw{ -90.0f };
	float	Pitch{ 0.0f };
	float	MoveSpeed{ 6.0f };
	float	TurnSpeed{ 0.5f };

	inline Matrix4x4 GetViewMatrix() { return Matrix4x4::LookAt(vecPosition, vecPosition + vecFront, vecUp); }

	void RecalculateCameraVectors();

	void UpdateCameraData(double deltaTime);
};

// END TEMP CAMERA