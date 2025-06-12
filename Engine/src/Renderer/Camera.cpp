#include "Camera.h"

#include "Core/Input.h"

void Camera_t::RecalculateCameraVectors()
{
	Vector3D front{};
	front.x = M_COS_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
	front.y = M_SIN_F(M_DEG2RAD_F(Pitch));
	front.z = M_SIN_F(M_DEG2RAD_F(Yaw)) * M_COS_F(M_DEG2RAD_F(Pitch));
	vecFront = front.Normalize();

	Vector3D newRight = Vector3D::CrossProduct(vecFront, vecWorldUp);
	vecRight = newRight.Normalize();

	Vector3D newUp = Vector3D::CrossProduct(vecRight, vecFront);
	vecUp = newUp.Normalize();
}

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