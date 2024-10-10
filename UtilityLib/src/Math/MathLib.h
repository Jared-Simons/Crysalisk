//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>
#include <cmath>

const f64 M_PI = 3.14159265359;
const f32 M_PI_F = 3.14159265359f;

f32 M_SQRT(f32 x);
f64 M_SQRT(f64 x);

f32 M_SIN(f32 x);
f64 M_SIN(f64 x);

f32 M_COS(f32 x);
f64 M_COS(f64 x);

f32 M_TAN(f32 x);
f64 M_TAN(f64 x);

f32 M_RAD2DEG(f32 angleInRadians);
f64 M_RAD2DEG(f64 angleInRadians);

f32 M_DEG2RAD(f32 angleInDegrees);
f64 M_DEG2RAD(f64 angleInDegrees);

f64 M_ABS(f64 x);
