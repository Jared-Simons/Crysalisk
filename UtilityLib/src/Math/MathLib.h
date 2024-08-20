//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <cmath>

#define M_PI 3.14159265359

#define M_SQRT(x) sqrt(x)
#define M_SIN(x) sin(x)
#define M_COS(x) cos(x)
#define M_TAN(x) tan(x)

#define M_RAD_TO_DEG(angleInRadians) angleInRadians * (180 / M_PI)
#define M_DEG_TO_RAD(angleInDegrees) angleInDegrees * (M_PI / 180)
