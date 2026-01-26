//=============================================================================
// Author: Jared Simons
// Date Created: 5/25/2025
// 
// Purpose: Camera types, typically used for rendering.
//=============================================================================
#pragma once

#include "Core/Input.h"
#include <Defines.h>
#include <Math/MathLib.h>

// TODO: Move these to their own file?
//-------------------------------------------------------------------------
// Purpose: A geometric plane.
//-------------------------------------------------------------------------
struct Plane
{
	Vector3D	vecNormal{};
	float		flDistance{};

	static inline Plane PlaneCreate(Vector3D pVecPoint, Vector3D pVecNormal)
	{
		Plane outPlane{};
		outPlane.vecNormal = pVecNormal.Normalize();
		outPlane.flDistance = Vector3D::Dot(outPlane.vecNormal, pVecPoint);
		return outPlane;
	}

	inline float GetSignedDistanceToPlane(Vector3D& pPoint)
	{
		return Vector3D::Dot(vecNormal, pPoint) - flDistance;
	}
};

//-------------------------------------------------------------------------
// Purpose: The different faces of a frustum.
//-------------------------------------------------------------------------
enum eFrustumPlanes
{
	TOP_FACE = 0,
	BOTTOM_FACE,
	RIGHT_FACE,
	LEFT_FACE,
	FAR_FACE,
	NEAR_FACE,

	MAX_FRUSTUM_PLANES
};

//-------------------------------------------------------------------------
// Purpose: A frustum, typically created by a camera and used for frustum-culling.
//-------------------------------------------------------------------------
struct Frustum
{
	// Can be indexed using the eFrustumPlanes enum.
	Plane	m_Faces[MAX_FRUSTUM_PLANES];

	inline bool	FrustumIntersectsAABB(Vector3D& pVecCenter, Vector3D& pVecExtents)
	{
		for (int iFrustumFace = 0; iFrustumFace < MAX_FRUSTUM_PLANES; iFrustumFace++)
		{
			float r = pVecExtents.x * M_ABS_F(m_Faces[iFrustumFace].vecNormal.x) +
				pVecExtents.y * M_ABS_F(m_Faces[iFrustumFace].vecNormal.y) +
				pVecExtents.z * M_ABS_F(m_Faces[iFrustumFace].vecNormal.z);

			//if (!(-r <= m_Faces[iFrustumFace].GetSignedDistanceToPlane(pVecCenter)))
			if (m_Faces[iFrustumFace].GetSignedDistanceToPlane(pVecCenter) <= -r)
				return false;
		}

		return true;
	}
};

//-------------------------------------------------------------------------
// A general purpose camera.
//-------------------------------------------------------------------------
struct Camera_t
{
	// The camera's position in world space.
	Vector vecPosition{ 0, 0, 3 };
	// The camera's current forward vector.
	Vector vecFront{ 0, 0, -1 };
	// The camera's current right vector.
	Vector vecRight{ 1, 0, 0 };
	// The camera's current up vector.
	Vector vecUp{ 0, 1, 0 };
	// The absolute up vector in world space.
	Vector vecWorldUp{ 0, 1, 0 };

	float	Yaw{ -90.0f };
	float	Pitch{ 0.0f };
	float	MoveSpeed{ 6.0f };
	float	TurnSpeed{ 0.5f };

	float	flFOV{ 75.0f };
	float	flZNear{ 0.0f };
	float	flZFar{ 10000.0f };
	float	flAspectRatio{ 16 / 9.0f };

	// Returns the camera's view matrix.
	CORE_API inline Matrix4x4 GetViewMatrix() { return Matrix4x4::LookAt(vecPosition, vecPosition + vecFront, vecUp); }

	// Recalculate's the camera's internal vectors, this is typically needed when the camera's rotation changes.
	CORE_API void RecalculateCameraVectors();

	// A simple update function.
	CORE_API void UpdateCameraData(class IInputSystem* pInputSystem, double deltaTime);

	// Creates and returns a frustum based on the camera's internal state.
	CORE_API Frustum CreateFrustumFromCamera() const;
};