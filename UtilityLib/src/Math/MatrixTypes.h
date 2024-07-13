//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: Adds Mathematical Matrix Utilities.
//=============================================================================
#pragma once

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
};

