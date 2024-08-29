//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <cmath>

#define M_PI 3.14159265359

inline double M_SQRT(float x) { return sqrt(x); }
inline float M_SQRTF(float x) { return sqrt(x); }

inline double M_SIN(float x) { return sin(x); }
inline float M_SINF(float x) { return sin(x); }

inline double M_COS(float x) { return cos(x); }
inline float M_COSF(float x) { return cos(x); }

inline double M_TAN(float x) { return tan(x); }
inline float M_TANF(float x) { return tan(x); }

#define M_RAD_TO_DEG(angleInRadians) angleInRadians * (180 / M_PI)
#define M_DEG_TO_RAD(angleInDegrees) angleInDegrees * (M_PI / 180)
