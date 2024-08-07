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
	float data[4][4];

	Matrix4x4()
	{
		data[0][0] = 0;
		data[0][1] = 0;
		data[0][2] = 0;
		data[0][3] = 0;

		data[1][0] = 0;
		data[1][1] = 0;
		data[1][2] = 0;
		data[1][3] = 0;

		data[2][0] = 0;
		data[2][1] = 0;
		data[2][2] = 0;
		data[2][3] = 0;

		data[3][0] = 0;
		data[3][1] = 0;
		data[3][2] = 0;
		data[3][3] = 0;
	}

	Matrix4x4(float initalValue)
	{
		data[0][0] = initalValue;
		data[0][1] = 0;
		data[0][2] = 0;
		data[0][3] = 0;

		data[1][0] = 0;
		data[1][1] = initalValue;
		data[1][2] = 0;
		data[1][3] = 0;

		data[2][0] = 0;
		data[2][1] = 0;
		data[2][2] = initalValue;
		data[2][3] = 0;

		data[3][0] = 0;
		data[3][1] = 0;
		data[3][2] = 0;
		data[3][3] = initalValue;
	}

	static Matrix4x4& Translate(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.data[0][3] += inVector.x;
		inMatrix.data[1][3] += inVector.y;
		inMatrix.data[2][3] += inVector.z;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, int valueToScaleBy)
	{
		inMatrix.data[0][0] *= valueToScaleBy;
		inMatrix.data[1][1] *= valueToScaleBy;
		inMatrix.data[2][2] *= valueToScaleBy;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.data[0][0] *= inVector.x;
		inMatrix.data[1][1] *= inVector.y;
		inMatrix.data[2][2] *= inVector.z;

		return inMatrix;
	}

	// TODO: Implement a quaternion type!
	static Matrix4x4& Rotate(Matrix4x4& inMatrix, float angle, Vector3D& axis)
	{
		inMatrix.data[0][0] = M_COS(angle) + (axis.x * axis.x) * (1 - M_COS(angle));
		inMatrix.data[0][1] = (axis.x * axis.y) * (1 - M_COS(angle) + (axis.z * M_SIN(angle)));
		inMatrix.data[0][2] = (axis.x * axis.z) * (1 - M_COS(angle) + (axis.y * M_SIN(angle)));
		inMatrix.data[0][3] = 0;

		inMatrix.data[1][0] = (axis.y * axis.x) * (1 - M_COS(angle) + (axis.z * M_SIN(angle)));
		inMatrix.data[1][1] = M_COS(angle) + (axis.y * axis.y) * (1 - M_COS(angle));
		inMatrix.data[1][2] = (axis.y * axis.z) * (1 - M_COS(angle)) - (axis.x * M_SIN(angle));
		inMatrix.data[1][3] = 0;

		inMatrix.data[2][0] = (axis.z * axis.x) * (1 - M_COS(angle)) - (axis.y * M_SIN(angle));
		inMatrix.data[2][1] = (axis.z * axis.y) * (1 - M_COS(angle)) + (axis.x * M_SIN(angle));
		inMatrix.data[2][2] = M_COS(angle) + (axis.z * axis.z) * (1 - M_COS(angle));
		inMatrix.data[2][3] = 0;

		inMatrix.data[3][0] = 0;
		inMatrix.data[3][1] = 0;
		inMatrix.data[3][2] = 0;
		inMatrix.data[3][3] = 1;

		return inMatrix;
	}

	static Matrix4x4 LookAt(Vector3D position, Vector3D target, Vector3D vecUp)
	{
		Matrix4x4 newMatrix(1.0f);

		// TODO: Hardcoded right vector

		newMatrix.data[0][0] = 1.0f;
		newMatrix.data[0][1] = 0.0f;
		newMatrix.data[0][2] = 0.0f;
		newMatrix.data[0][3] = -position.x;

		newMatrix.data[1][0] = vecUp.x;
		newMatrix.data[1][1] = vecUp.y;
		newMatrix.data[1][2] = vecUp.z;
		newMatrix.data[1][3] = -position.y;

		newMatrix.data[2][0] = target.x;
		newMatrix.data[2][1] = target.y;
		newMatrix.data[2][2] = target.z;
		newMatrix.data[2][3] = -position.z;

		newMatrix.data[3][0] = 0;
		newMatrix.data[3][1] = 0;
		newMatrix.data[3][2] = 0;
		newMatrix.data[3][3] = 1;

		return newMatrix;
	}

	// Reference for matrix calculation: https://www.songho.ca/opengl/gl_projectionmatrix.html#fov
	// also: https://jsantell.com/3d-projection/
	static Matrix4x4& Perspective(float fov, float aspectRatio, float nearClippingPlane, float farClippingPlane)
	{
		Matrix4x4 newMatrix(1.0f);
		float right = 0.0f;
		float left = 0.0f;
		float top = 0.0f;
		float bottom = 0.0f;

		// First calculate the primitive values for plane creation
		top = nearClippingPlane * M_TAN(fov / 2);
		bottom = -top;
		right = aspectRatio * top;
		left = -right;

		newMatrix.data[0][0] = (2.0f * nearClippingPlane) / (right - left);
		newMatrix.data[0][1] = 0;
		newMatrix.data[0][2] = (right + left) / (right - left);
		newMatrix.data[0][3] = 0;

		newMatrix.data[1][0] = 0;
		newMatrix.data[1][1] = (2.0 * nearClippingPlane) / (top - bottom);
		newMatrix.data[1][2] = (top + bottom) / (top - bottom);
		newMatrix.data[1][3] = 0;

		newMatrix.data[2][0] = 0;
		newMatrix.data[2][1] = 0;
		newMatrix.data[2][2] = (farClippingPlane + nearClippingPlane) / (nearClippingPlane - farClippingPlane);
		newMatrix.data[2][3] = (2.0f * farClippingPlane * nearClippingPlane) / (nearClippingPlane - farClippingPlane);

		newMatrix.data[3][0] = 0;
		newMatrix.data[3][1] = 0;
		newMatrix.data[3][2] = -1;
		newMatrix.data[3][3] = 0;

		return newMatrix;
	}
};

