//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================

#include "MathLib.h"

#include <cmath>

f32 M_SQRT(f32 x)
{
	return sqrt(x);
}

f64 M_SQRT(f64 x)
{
	return sqrt(x);
}

f32 M_SIN(f32 x)
{
	return sin(x);
}

f64 M_SIN(f64 x)
{
	return sin(x);
}

f32 M_COS(f32 x)
{
	return cos(x);
}

f64 M_COS(f64 x)
{
	return cos(x);
}

f32 M_TAN(f32 x)
{
	return tan(x);
}

f64 M_TAN(f64 x)
{
	return tan(x);
}

f32 M_RAD2DEG(f32 angleInRadians)
{
	return angleInRadians * (180.0f / M_PI_F);
}

f64 M_RAD2DEG(f64 angleInRadians)
{
	return angleInRadians * (180.0 / M_PI);
}

f32 M_DEG2RAD(f32 angleInDegrees)
{
	return angleInDegrees * (M_PI_F / 180.0f);
}

f64 M_DEG2RAD(f64 angleInDegrees)
{
	return angleInDegrees * (M_PI / 180.0);
}

f64 M_ABS(f64 x)
{
	return abs(x);
}