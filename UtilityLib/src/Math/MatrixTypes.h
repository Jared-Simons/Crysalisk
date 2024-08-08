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
		Vector3D zAxis;

		zAxis.x = target.x - position.x;
		zAxis.y = target.y - position.y;
		zAxis.z = target.z - position.z;

		zAxis.NormalizeInPlace();
		Vector3D xAxis = Vector3D::CrossProduct(zAxis, vecUp);
		xAxis.NormalizeInPlace();

		Vector3D yAxis = Vector3D::CrossProduct(xAxis, zAxis);

		newMatrix.data[0][0] = xAxis.x;
		newMatrix.data[1][0] = yAxis.x;
		newMatrix.data[2][0] = -zAxis.x;
		newMatrix.data[3][0] = 0;

		newMatrix.data[0][1] = xAxis.y;
		newMatrix.data[1][1] = yAxis.y;
		newMatrix.data[2][1] = -zAxis.y;
		newMatrix.data[3][1] = 0;

		newMatrix.data[0][2] = xAxis.z;
		newMatrix.data[1][2] = yAxis.z;
		newMatrix.data[2][2] = -zAxis.z;
		newMatrix.data[3][2] = 0;

		newMatrix.data[0][3] = -Vector3D::DotProduct(xAxis, position);
		newMatrix.data[1][3] = -Vector3D::DotProduct(yAxis, position);
		newMatrix.data[2][3] = Vector3D::DotProduct(zAxis, position);
		newMatrix.data[3][3] = 1.0f;

		return newMatrix;
	}

	static Matrix4x4& Perspective(float fov, float aspectRatio, float nearClippingPlane, float farClippingPlane)
	{
		Matrix4x4 newMatrix(0.0f);
		float halfTanFOV = M_TAN(fov * 0.5f);

		newMatrix.data[0][0] = 1.0f / (aspectRatio * halfTanFOV);
		newMatrix.data[1][1] = 1.0f / halfTanFOV;
		newMatrix.data[2][2] = -((farClippingPlane + nearClippingPlane) / (farClippingPlane - nearClippingPlane));
		newMatrix.data[3][2] = -1.0f;
		newMatrix.data[2][3] = -((2.0f * farClippingPlane * nearClippingPlane) / (farClippingPlane - nearClippingPlane));

		return newMatrix;
	}
};