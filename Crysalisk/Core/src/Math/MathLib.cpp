//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================

#include "MathLib.h"

#include <math.h>

float M_SQRT_F(float x)
{
	return sqrt(x);
}

double M_SQRT(double x)
{
	return sqrt(x);
}

float M_SIN_F(float x)
{
	return sin(x);
}

double M_SIN(double x)
{
	return sin(x);
}

float M_COS_F(float x)
{
	return cos(x);
}

double M_COS(double x)
{
	return cos(x);
}

float M_TAN_F(float x)
{
	return tan(x);
}

double M_TAN(double x)
{
	return tan(x);
}

float M_RAD2DEG_F(float angleInRadians)
{
	return angleInRadians * (180.0f / kPI_F);
}

double M_RAD2DEG(double angleInRadians)
{
	return angleInRadians * (180.0 / kPI);
}

float M_DEG2RAD_F(float angleInDegrees)
{
	return angleInDegrees * (kPI_F / 180.0f);
}

double M_DEG2RAD(double angleInDegrees)
{
	return angleInDegrees * (kPI / 180.0);
}

float M_ABS_F(float x)
{
	return fabsf(x);
}

double M_ABS(double x)
{
	return fabsl(x);
}