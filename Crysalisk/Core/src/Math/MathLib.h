//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>
#include <cmath>

const double kPI = 3.14159265359;
const float kPI_F = 3.14159265359f;

DLL_EXPORT float M_SQRT_F(float x);
DLL_EXPORT double M_SQRT(double x);

DLL_EXPORT float M_SIN_F(float x);
DLL_EXPORT double M_SIN(double x);

DLL_EXPORT float M_COS_F(float x);
DLL_EXPORT double M_COS(double x);

DLL_EXPORT float M_TAN_F(float x);
DLL_EXPORT double M_TAN(double x);

DLL_EXPORT float M_RAD2DEG_F(float angleInRadians);
DLL_EXPORT double M_RAD2DEG(double angleInRadians);

DLL_EXPORT float M_DEG2RAD_F(float angleInDegrees);
DLL_EXPORT double M_DEG2RAD(double angleInDegrees);

DLL_EXPORT float M_ABS_F(float x);
DLL_EXPORT double M_ABS(double x);
