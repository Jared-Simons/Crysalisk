//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================

#include "MathLib.h"
#include "Ray.h"

#include "Core/Assertions.h"
#include "Renderer/Camera.h"

#include <math.h>

f32 M_SQRT_F(f32 x)
{
	return sqrtf(x);
}

f64 M_SQRT(f64 x)
{
	return sqrt(x);
}

f32 M_SIN_F(f32 x)
{
	return sinf(x);
}

f64 M_SIN(f64 x)
{
	return sin(x);
}

f32 M_COS_F(f32 x)
{
	return cosf(x);
}

f64 M_COS(f64 x)
{
	return cos(x);
}

f32 M_TAN_F(f32 x)
{
	return tanf(x);
}

f64 M_TAN(f64 x)
{
	return tan(x);
}

f32 M_ATAN2_F(f32 x, f32 y)
{
	return atan2f(x, y);
}

f64 M_ATAN2(f64 x, f64 y)
{
	return atan2(x, y);
}

f32 M_RAD2DEG_F(f32 angleInRadians)
{
	return angleInRadians * (180.0f / kPI_F);
}

f64 M_RAD2DEG(f64 angleInRadians)
{
	return angleInRadians * (180.0 / kPI);
}

f32 M_DEG2RAD_F(f32 angleInDegrees)
{
	return angleInDegrees * (kPI_F / 180.0f);
}

f64 M_DEG2RAD(f64 angleInDegrees)
{
	return angleInDegrees * (kPI / 180.0);
}

f32 M_ABS_F(f32 x)
{
	return fabsf(x);
}

f64 M_ABS(f64 x)
{
	return fabsl(x);
}

bool RayIntersectsPlane(Ray_t* pRay, Plane* pPlane)
{
	CASSERTMSG(pRay && pPlane, "RayIntersectsPlane() requires pointers to a ray and plane!");

	// Ray's direction is already normalized.
	f32 denom = Vector::Dot(pPlane->vecNormal, pRay->vecDirection);
	f32 num = pPlane->flDistance - Vector::Dot(pRay->vecOrigin, pPlane->vecNormal);
	f32 t = num / denom;

	if (t >= 0.0f)
	{
		pRay->bDidHit = true;
		pRay->flDistance = t;
		pRay->vecEnd = pRay->vecOrigin + (pRay->vecDirection * t);

		return true;
	}

	return false;
}