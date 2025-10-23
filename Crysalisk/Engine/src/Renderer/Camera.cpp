//=============================================================================
// Author: Jared Simons
// Date Created: 5/25/2025
// 
// Purpose: Camera types, typically used for rendering.
//=============================================================================
#include "Camera.h"

#include "Core/Engine.h"
#include "Core/Input.h"

//-------------------------------------------------------------------------
// Purpose: Recalculate's the camera's internal vectors, 
//			this is typically needed when the camera's rotation changes.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void Camera_t::RecalculateCameraVectors()
{
	Vector3D_OLD front{};
	front.x = M_COS_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
	front.y = M_SIN_F(M_DEG2RAD_F(Pitch));
	front.z = M_SIN_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
	vecFront = front.Normalize();

	Vector3D_OLD newRight = Vector3D_OLD::CrossProduct(vecFront, vecWorldUp);
	vecRight = newRight.Normalize();

	Vector3D_OLD newUp = Vector3D_OLD::CrossProduct(vecRight, vecFront);
	vecUp = newUp.Normalize();

	// Recalculate the aspect ratio in case the resolution changed.
	// TODO: This should probably be triggered any time the screen size changes.
	unsigned int iWindowWidth, iWindowHeight;
	g_pEngine->GetEngineWindow()->GetWindowSize(iWindowWidth, iWindowHeight);
	flAspectRatio = (iWindowWidth / (float)iWindowHeight);
}

//-------------------------------------------------------------------------
// Purpose: A simple update function.
// Input:	deltaTime - The time elapsed since this function was last called.
// Output:	none.
//-------------------------------------------------------------------------
void Camera_t::UpdateCameraData(double deltaTime)
{
	float velocity = (float)(2.5f * deltaTime);
	const float cameraSpeedMultiplier = 100.0;
	const float cameraRotationSpeedMultipler = 50.0f;

	if (g_pInputSystem_Internal->IsKeyPressed(KEY_W))
	{
		vecPosition += (vecFront * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_S))
	{
		vecPosition -= (vecFront * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_A))
	{
		vecPosition -= (vecRight * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_D))
	{
		vecPosition += (vecRight * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_SPACE))
	{
		vecPosition += (vecUp * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_CTRL))
	{
		vecPosition -= (vecUp * (velocity * cameraSpeedMultiplier));
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_RIGHT))
	{
		Yaw += velocity * cameraRotationSpeedMultipler;
		RecalculateCameraVectors();
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_LEFT))
	{
		Yaw -= velocity * cameraRotationSpeedMultipler;
		RecalculateCameraVectors();
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_UP))
	{
		Pitch += velocity * cameraRotationSpeedMultipler;

		if (Pitch > 89.0f)
			Pitch = 89.0f;

		RecalculateCameraVectors();
	}
	if (g_pInputSystem_Internal->IsKeyPressed(KEY_DOWN))
	{
		Pitch -= velocity * cameraRotationSpeedMultipler;

		if (Pitch < -89.0f)
			Pitch = -89.0f;

		RecalculateCameraVectors();
	}
}

//-------------------------------------------------------------------------
// Purpose: Creates and returns a frustum based on the camera's internal state.
// Input:	none.
// Output:	A new frustum that contains the camera's viewing volume.
//-------------------------------------------------------------------------
Frustum Camera_t::CreateFrustumFromCamera()
{
	Frustum outFrustum{};
	const float flHalfVSide = flZFar * M_TAN_F(M_DEG2RAD_F(flFOV) * 0.5f);
	const float flHalfHSide = flHalfVSide * flAspectRatio;
	Vector3D_OLD vecFrontFar = vecFront * flZFar;

	outFrustum.m_Faces[NEAR_FACE] = Plane::PlaneCreate(vecPosition, Vector3D_OLD::CrossProduct(vecFrontFar + (vecUp * flHalfVSide), vecRight));
	outFrustum.m_Faces[FAR_FACE] = Plane::PlaneCreate(vecPosition, Vector3D_OLD::CrossProduct(vecRight, vecFrontFar - (vecUp * flHalfVSide)));
	outFrustum.m_Faces[RIGHT_FACE] = Plane::PlaneCreate(vecPosition, Vector3D_OLD::CrossProduct(vecUp, vecFrontFar + (vecRight * flHalfHSide)));
	outFrustum.m_Faces[LEFT_FACE] = Plane::PlaneCreate(vecPosition, Vector3D_OLD::CrossProduct(vecFrontFar - (vecRight * flHalfHSide), vecUp));
	outFrustum.m_Faces[TOP_FACE] = Plane::PlaneCreate((vecFront * flZNear) + vecPosition, vecFront);
	outFrustum.m_Faces[BOTTOM_FACE] = Plane::PlaneCreate(vecPosition + vecFrontFar, vecFront * -1.0f);

	return outFrustum;
}
