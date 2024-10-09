//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: Adds Mathematical Matrix Utilities.
//=============================================================================
#pragma once

#include <Defines.h>

#include "MathLib.h"
#include "VectorTypes.h"

class Matrix2x2
{
public:
	f32 r1[2];
	f32 r2[2];

	Matrix2x2()
		: r1{ 0 }
		, r2{ 0 } {}

	Matrix2x2(const f32 r00, const f32 r01, const f32 r10, const f32 r11)
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

	Matrix2x2 operator*(const f32 scalar)
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

/*
class Matrix4x4
{
public:
	f32 data[4][4];

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

	Matrix4x4(f32 initalValue)
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

	static Matrix4x4& Translate(Matrix4x4& inMatrix, const Vector3D& inVector)
	{
		inMatrix.data[3][0] += inVector.x;
		inMatrix.data[3][1] += inVector.y;
		inMatrix.data[3][2] += inVector.z;
		inMatrix.data[3][3] = 1.0f;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, i32 valueToScaleBy)
	{
		inMatrix.data[0][0] *= valueToScaleBy;
		inMatrix.data[1][1] *= valueToScaleBy;
		inMatrix.data[2][2] *= valueToScaleBy;
		inMatrix.data[3][3] = 1.0f;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.data[0][0] *= inVector.x;
		inMatrix.data[1][1] *= inVector.y;
		inMatrix.data[2][2] *= inVector.z;
		inMatrix.data[3][3] = 1.0f;

		return inMatrix;
	}

	// TODO: Implement a quaternion type!
	static Matrix4x4& Rotate(Matrix4x4& inMatrix, f32 angle, Vector3D& axis)
	{
		inMatrix.data[0][0] = M_COS(angle) + (axis.x * axis.x) * (1 - M_COS(angle));
		inMatrix.data[0][1] = (axis.y * axis.x) * (1 - M_COS(angle)) + (axis.z * M_SIN(angle));
		inMatrix.data[0][2] = (axis.z * axis.x) * (1 - M_COS(angle)) - (axis.y * M_SIN(angle));
		inMatrix.data[0][3] = 0.0f;

		inMatrix.data[1][0] = (axis.x * axis.y) * (1 - M_COS(angle)) - (axis.z * M_SIN(angle));
		inMatrix.data[1][1] = M_COS(angle) + (axis.y * axis.y) * (1 - M_COS(angle));
		inMatrix.data[1][2] = (axis.z * axis.y) * (1 - M_COS(angle)) + (axis.x * M_SIN(angle));
		inMatrix.data[1][3] = 0.0f;

		inMatrix.data[2][0] = (axis.x * axis.z) * (1 - M_COS(angle)) + (axis.y * M_SIN(angle));
		inMatrix.data[2][1] = (axis.y * axis.z) * (1 - M_COS(angle)) - (axis.x * M_SIN(angle));
		inMatrix.data[2][2] = M_COS(angle) + (axis.z * axis.z) * (1 - M_COS(angle));
		inMatrix.data[2][3] = 0.0f;

		inMatrix.data[3][0] = 0.0f;
		inMatrix.data[3][1] = 0.0f;
		inMatrix.data[3][2] = 0.0f;
		inMatrix.data[3][3] = 1.0f;

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
		newMatrix.data[0][1] = yAxis.x;
		newMatrix.data[0][2] = -zAxis.x;
		newMatrix.data[0][3] = 0;

		newMatrix.data[1][0] = xAxis.y;
		newMatrix.data[1][1] = yAxis.y;
		newMatrix.data[1][2] = -zAxis.y;
		newMatrix.data[1][3] = 0;

		newMatrix.data[2][0] = xAxis.z;
		newMatrix.data[2][1] = yAxis.z;
		newMatrix.data[2][2] = -zAxis.z;
		newMatrix.data[2][3] = 0;

		newMatrix.data[3][0] = -Vector3D::DotProduct(xAxis, position);
		newMatrix.data[3][1] = -Vector3D::DotProduct(yAxis, position);
		newMatrix.data[3][2] = Vector3D::DotProduct(zAxis, position);
		newMatrix.data[3][3] = 1.0f;

		return newMatrix;
	}

	static Matrix4x4 Perspective(f32 fov, f32 aspectRatio, f32 nearClippingPlane, f32 farClippingPlane)
	{
		Matrix4x4 newMatrix(0.0f);
		f32 halfTanFOV = M_TAN(fov * 0.5f);

		newMatrix.data[0][0] = 1.0f / (aspectRatio * halfTanFOV);
		newMatrix.data[1][1] = 1.0f / halfTanFOV;
		newMatrix.data[2][2] = -((farClippingPlane + nearClippingPlane) / (farClippingPlane - nearClippingPlane));
		newMatrix.data[2][3] = -1.0f;
		newMatrix.data[3][2] = -((2.0f * farClippingPlane * nearClippingPlane) / (farClippingPlane - nearClippingPlane));

		return newMatrix;
	}
};*/

class Matrix4x4
{
public:
	//f32 data[4][4];
	f32 data[16];

	Matrix4x4()
	{
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		data[4] = 0;
		data[5] = 0;
		data[6] = 0;
		data[7] = 0;

		data[8] = 0;
		data[9] = 0;
		data[10] = 0;
		data[11] = 0;

		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = 0;
	}

	Matrix4x4(f32 initalValue)
	{
		data[0] = initalValue;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		data[4] = 0;
		data[5] = initalValue;
		data[6] = 0;
		data[7] = 0;

		data[8] = 0;
		data[9] = 0;
		data[10] = initalValue;
		data[11] = 0;

		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = initalValue;
	}

	static Matrix4x4& Translate(Matrix4x4& inMatrix, const Vector3D& inVector)
	{
		inMatrix.data[12] += inVector.x;
		inMatrix.data[13] += inVector.y;
		inMatrix.data[14] += inVector.z;
		inMatrix.data[15] = 1.0f;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, i32 valueToScaleBy)
	{
		inMatrix.data[0] *= valueToScaleBy;
		inMatrix.data[5] *= valueToScaleBy;
		inMatrix.data[10] *= valueToScaleBy;
		inMatrix.data[15] = 1.0f;

		return inMatrix;
	}

	static Matrix4x4& Scale(Matrix4x4& inMatrix, Vector3D& inVector)
	{
		inMatrix.data[0] *= inVector.x;
		inMatrix.data[5] *= inVector.y;
		inMatrix.data[10] *= inVector.z;
		inMatrix.data[15] = 1.0f;

		return inMatrix;
	}

	// TODO: Implement a quaternion type!
	static Matrix4x4& Rotate(Matrix4x4& inMatrix, f32 angle, Vector3D& axis)
	{
		inMatrix.data[0] = M_COS(angle) + (axis.x * axis.x) * (1 - M_COS(angle));
		inMatrix.data[1] = (axis.y * axis.x) * (1 - M_COS(angle)) + (axis.z * M_SIN(angle));
		inMatrix.data[2] = (axis.z * axis.x) * (1 - M_COS(angle)) - (axis.y * M_SIN(angle));
		inMatrix.data[3] = 0.0f;

		inMatrix.data[4] = (axis.x * axis.y) * (1 - M_COS(angle)) - (axis.z * M_SIN(angle));
		inMatrix.data[5] = M_COS(angle) + (axis.y * axis.y) * (1 - M_COS(angle));
		inMatrix.data[6] = (axis.z * axis.y) * (1 - M_COS(angle)) + (axis.x * M_SIN(angle));
		inMatrix.data[7] = 0.0f;

		inMatrix.data[8] = (axis.x * axis.z) * (1 - M_COS(angle)) + (axis.y * M_SIN(angle));
		inMatrix.data[9] = (axis.y * axis.z) * (1 - M_COS(angle)) - (axis.x * M_SIN(angle));
		inMatrix.data[10] = M_COS(angle) + (axis.z * axis.z) * (1 - M_COS(angle));
		inMatrix.data[11] = 0.0f;

		inMatrix.data[12] = 0.0f;
		inMatrix.data[13] = 0.0f;
		inMatrix.data[14] = 0.0f;
		inMatrix.data[15] = 1.0f;

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

		newMatrix.data[0] = xAxis.x;
		newMatrix.data[1] = yAxis.x;
		newMatrix.data[2] = -zAxis.x;
		newMatrix.data[3] = 0;

		newMatrix.data[4] = xAxis.y;
		newMatrix.data[5] = yAxis.y;
		newMatrix.data[6] = -zAxis.y;
		newMatrix.data[7] = 0;

		newMatrix.data[8] = xAxis.z;
		newMatrix.data[9] = yAxis.z;
		newMatrix.data[10] = -zAxis.z;
		newMatrix.data[11] = 0;

		newMatrix.data[12] = -Vector3D::DotProduct(xAxis, position);
		newMatrix.data[13] = -Vector3D::DotProduct(yAxis, position);
		newMatrix.data[14] = Vector3D::DotProduct(zAxis, position);
		newMatrix.data[15] = 1.0f;

		return newMatrix;
	}

	static Matrix4x4 Perspective(f32 fov, f32 aspectRatio, f32 nearClippingPlane, f32 farClippingPlane)
	{
		Matrix4x4 newMatrix(0.0f);
		f32 halfTanFOV = M_TAN(fov * 0.5f);

		newMatrix.data[0] = 1.0f / (aspectRatio * halfTanFOV);
		newMatrix.data[5] = 1.0f / halfTanFOV;
		newMatrix.data[10] = -((farClippingPlane + nearClippingPlane) / (farClippingPlane - nearClippingPlane));
		newMatrix.data[11] = -1.0f;
		newMatrix.data[14] = -((2.0f * farClippingPlane * nearClippingPlane) / (farClippingPlane - nearClippingPlane));

		return newMatrix;
	}
};