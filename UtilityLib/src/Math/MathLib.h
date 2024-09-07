//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <cmath>

#define M_PI 3.14159265359

inline double M_SQRT(double x) { return sqrt(x); }
inline float M_SQRT(float x) { return sqrt(x); }

inline double M_SIN(double x) { return sin(x); }
inline float M_SIN(float x) { return sin(x); }

inline double M_COS(double x) { return cos(x); }
inline float M_COS(float x) { return cos(x); }

inline double M_TAN(double x) { return tan(x); }
inline float M_TAN(float x) { return tan(x); }

inline double M_RAD2DEG(double angleInRadians) { return angleInRadians * (180.0 / M_PI); }
inline float M_RAD2DEG(float angleInRadians) { return angleInRadians * (180.0f / static_cast<float>(M_PI)); }

inline double M_DEG2RAD(double angleInDegrees) { return angleInDegrees * (M_PI / 180.0); }
inline float M_DEG2RAD(float angleInDegrees) { return angleInDegrees * (static_cast<float>(M_PI)) / 180.0f; }

inline double M_ABS(double x) { return abs(x); }
