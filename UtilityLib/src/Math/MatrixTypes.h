//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: Adds Mathematical Matrix Utilities.
//=============================================================================
#pragma once

#include "MathLib.h"
#include "VectorTypes.h"

class Matrix2x2
{
public:
	float r1[2];
	float r2[2];

	Matrix2x2()
		: r1{ 0 }
		, r2{ 0 } {}

	Matrix2x2(const float r00, const float r01, const float r10, const float r11)
	{
		r1[0] = r00;
		r1[1] = r01;
		r2[0] = r10;
		r2[1] = r11;
	}

	Matrix2x2 operator+(const Matrix2x2& otherMatrix)
	{
		Matrix2x2 outMatrix;
		outMatrix.r1[0] = r1[0] + otherMatrix.r1[0];
		outMatrix.r1[1] = r1[1] + otherMatrix.r1[1];
		outMatrix.r2[0] = r2[0] + otherMatrix.r2[0];
		outMatrix.r2[1] = r2[1] + otherMatrix.r2[1];
		return outMatrix;
	}

	Matrix2x2 operator-(const Matrix2x2& otherMatrix)
	{
		Matrix2x2 outMatrix;
		outMatrix.r1[0] = r1[0] - otherMatrix.r1[0];
		outMatrix.r1[1] = r1[1] - otherMatrix.r1[1];
		outMatrix.r2[0] = r2[0] - otherMatrix.r2[0];
		outMatrix.r2[1] = r2[1] - otherMatrix.r2[1];
		return outMatrix;
	}

	Matrix2x2 operator*(const float scalar)
	{
		Matrix2x2 outMatrix;
		outMatrix.r1[0] = r1[0] * scalar;
		outMatrix.r1[1] = r1[1] * scalar;
		outMatrix.r2[0] = r2[0] * scalar;
		outMatrix.r2[1] = r2[1] * scalar;
		return outMatrix;
	}

	Matrix2x2 operator*(const Matrix2x2& otherMatrix)
	{
		Matrix2x2 outMatrix;

		outMatrix.r1[0] = (r1[0] * otherMatrix.r1[0]) + (r1[1] * otherMatrix.r2[0]);
		outMatrix.r1[1] = (r1[0] * otherMatrix.r1[1]) + (r1[1] * otherMatrix.r2[1]);
		outMatrix.r2[0] = (r2[0] * otherMatrix.r1[0]) + (r2[1] * otherMatrix.r2[0]);
		outMatrix.r2[1] = (r2[0] * otherMatrix.r1[1]) + (r2[1] * otherMatrix.r2[1]);

		return outMatrix;
	}

};

class Matrix4x4
{
public:
	float r1[4];
	float r2[4];
	float r3[4];
	float r4[4];

	Matrix4x4()
	{
		r1[0] = 0;
		r1[1] = 0;
		r1[2] = 0;
		r1[3] = 0;

		r2[0] = 0;
		r2[1] = 0;
		r2[2] = 0;
		r2[3] = 0;

		r3[0] = 0;
		r3[1] = 0;
		r3[2] = 0;
		r3[3] = 0;

		r4[0] = 0;
		r4[1] = 0;
		r4[2] = 0;
		r4[3] = 0;
	}

	Matrix4x4(float initalValue)
	{
		r1[0] = initalValue;
		r1[1] = 0;
		r1[2] = 0;
		r1[3] = 0;

		r2[0] = 0;
		r2[1] = initalValue;
		r2[2] = 0;
		r2[3] = 0;

		r3[0] = 0;
		r3[1] = 0;
		r3[2] = initalValue;
		r3[3] = 0;

		r4[0] = 0;
		r4[1] = 0;
		r4[2] = 0;
		r4[3] = initalValue;
	}

	static Matrix4x4& Translate(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.r1[3] += inVector.x;
		inMatrix.r2[3] += inVector.y;
		inMatrix.r3[3] += inVector.z;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, int valueToScaleBy)
	{
		inMatrix.r1[0] *= valueToScaleBy;
		inMatrix.r2[1] *= valueToScaleBy;
		inMatrix.r3[2] *= valueToScaleBy;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.r1[0] *= inVector.x;
		inMatrix.r2[1] *= inVector.y;
		inMatrix.r3[2] *= inVector.z;

		return inMatrix;
	}

	// TODO: Implement a quaternion type!
	static Matrix4x4& Rotate(Matrix4x4& inMatrix, float angle, Vector3D& axis)
	{
		inMatrix.r1[0] = M_COS(angle) + (axis.x * axis.x) * (1 - M_COS(angle));
		inMatrix.r1[1] = (axis.x * axis.y) * (1 - M_COS(angle) + (axis.z * M_SIN(angle)));
		inMatrix.r1[2] = (axis.x * axis.z) * (1 - M_COS(angle) + (axis.y * M_SIN(angle)));
		inMatrix.r1[3] = 0;

		inMatrix.r2[0] = (axis.y * axis.x) * (1 - M_COS(angle) + (axis.z * M_SIN(angle)));
		inMatrix.r2[1] = M_COS(angle) + (axis.y * axis.y) * (1 - M_COS(angle));
		inMatrix.r2[2] = (axis.y * axis.z) * (1 - M_COS(angle)) - (axis.x * M_SIN(angle));
		inMatrix.r2[3] = 0;

		inMatrix.r3[0] = (axis.z * axis.x) * (1 - M_COS(angle)) - (axis.y * M_SIN(angle));
		inMatrix.r3[1] = (axis.z * axis.y) * (1 - M_COS(angle)) + (axis.x * M_SIN(angle));
		inMatrix.r3[2] = M_COS(angle) + (axis.z * axis.z) * (1 - M_COS(angle));
		inMatrix.r3[3] = 0;

		inMatrix.r4[0] = 0;
		inMatrix.r4[1] = 0;
		inMatrix.r4[2] = 0;
		inMatrix.r4[3] = 1;

		return inMatrix;
	}
};

