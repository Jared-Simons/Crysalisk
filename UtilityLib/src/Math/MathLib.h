//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>
#include <cmath>

#define M_PI 3.14159265359
#define M_PI_F 3.14159265359f

inline f64 M_SQRT(f64 x) { return sqrt(x); }
inline f32 M_SQRT(f32 x) { return sqrt(x); }

inline f64 M_SIN(f64 x) { return sin(x); }
inline f32 M_SIN(f32 x) { return sin(x); }

inline f64 M_COS(f64 x) { return cos(x); }
inline f32 M_COS(f32 x) { return cos(x); }

inline f64 M_TAN(f64 x) { return tan(x); }
inline f32 M_TAN(f32 x) { return tan(x); }

inline f64 M_RAD2DEG(f64 angleInRadians) { return angleInRadians * (180.0 / M_PI); }
inline f32 M_RAD2DEG(f32 angleInRadians) { return angleInRadians * (180.0f / static_cast<f32>(M_PI)); }

inline f64 M_DEG2RAD(f64 angleInDegrees) { return angleInDegrees * (M_PI / 180.0); }
inline f32 M_DEG2RAD(f32 angleInDegrees) { return angleInDegrees * (M_PI_F / 180.0f); }

inline f64 M_ABS(f64 x) { return abs(x); }
