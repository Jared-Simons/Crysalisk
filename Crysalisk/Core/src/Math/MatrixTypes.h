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
	//float data[4][4];
	float data[16];

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

	Matrix4x4(float initalValue)
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

	Matrix4x4 operator*(Matrix4x4& pOtherMatrix)
	{
		Matrix4x4 outMatrix{};

		float* mat1Ptr = &data[0];
		float* mat2Ptr = &pOtherMatrix.data[0];

		for (int firstMat = 0; firstMat < 4; firstMat++)
		{
			for (int secondMat = 0; secondMat < 4; secondMat++)
			{
				outMatrix.data[(firstMat * 4) + secondMat] = 
					  (mat1Ptr[(firstMat * 4)] * mat2Ptr[secondMat]) 
					+ (mat1Ptr[(firstMat * 4) + 1] * mat2Ptr[secondMat + 4]) 
					+ (mat1Ptr[(firstMat * 4) + 2] * mat2Ptr[secondMat + 8]) 
					+ (mat1Ptr[(firstMat * 4) + 3] * mat2Ptr[secondMat + 12]);
			}
		}

		return outMatrix;
	}

	Matrix4x4 operator *=(Matrix4x4& pOtherMatrix)
	{
		return *this * pOtherMatrix;
	}

	Vector4D operator*(Vector4D& pVec)
	{
		Vector4D outVector{};

		// TODO: We could wrap this in a loop if we made our vector elements into an array.
		outVector.x = data[0] * pVec.x;
		outVector.x += data[1] * pVec.y;
		outVector.x += data[2] * pVec.z;
		outVector.x += data[3] * pVec.w;

		outVector.y = data[4] * pVec.x;
		outVector.y += data[5] * pVec.y;
		outVector.y += data[6] * pVec.z;
		outVector.y += data[7] * pVec.w;

		outVector.z = data[8] * pVec.x;
		outVector.z += data[9] * pVec.y;
		outVector.z += data[10] * pVec.z;
		outVector.z += data[11] * pVec.w;

		outVector.w = data[12] * pVec.x;
		outVector.w += data[13] * pVec.y;
		outVector.w += data[14] * pVec.z;
		outVector.w += data[15] * pVec.w;

		return outVector;
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

	static Matrix4x4& Rotate(Matrix4x4& inMatrix, float angle, Vector3D& axis)
	{
		inMatrix.data[0] = M_COS_F(angle) + (axis.x * axis.x) * (1 - M_COS_F(angle));
		inMatrix.data[1] = (axis.y * axis.x) * (1 - M_COS_F(angle)) + (axis.z * M_SIN_F(angle));
		inMatrix.data[2] = (axis.z * axis.x) * (1 - M_COS_F(angle)) - (axis.y * M_SIN_F(angle));
		inMatrix.data[3] = 0.0f;

		inMatrix.data[4] = (axis.x * axis.y) * (1 - M_COS_F(angle)) - (axis.z * M_SIN_F(angle));
		inMatrix.data[5] = M_COS_F(angle) + (axis.y * axis.y) * (1 - M_COS_F(angle));
		inMatrix.data[6] = (axis.z * axis.y) * (1 - M_COS_F(angle)) + (axis.x * M_SIN_F(angle));
		inMatrix.data[7] = 0.0f;

		inMatrix.data[8] = (axis.x * axis.z) * (1 - M_COS_F(angle)) + (axis.y * M_SIN_F(angle));
		inMatrix.data[9] = (axis.y * axis.z) * (1 - M_COS_F(angle)) - (axis.x * M_SIN_F(angle));
		inMatrix.data[10] = M_COS_F(angle) + (axis.z * axis.z) * (1 - M_COS_F(angle));
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

	static Matrix4x4 Perspective(float fov, float aspectRatio, float nearClippingPlane, float farClippingPlane)
	{
		Matrix4x4 newMatrix(0.0f);
		float halfTanFOV = M_TAN_F(fov * 0.5f);

		newMatrix.data[0] = 1.0f / (aspectRatio * halfTanFOV);
		newMatrix.data[5] = 1.0f / halfTanFOV;
		newMatrix.data[10] = -((farClippingPlane + nearClippingPlane) / (farClippingPlane - nearClippingPlane));
		newMatrix.data[11] = -1.0f;
		newMatrix.data[14] = -((2.0f * farClippingPlane * nearClippingPlane) / (farClippingPlane - nearClippingPlane));

		return newMatrix;
	}

	static Matrix4x4 Ortho(float left, float right, float bottom, float top, float near, float far)
	{
		Matrix4x4 newMatrix(1.0f);

		float lr = 1.0f / (left - right);
		float bt = 1.0f / (bottom - top);
		float nf = 1.0f / (near - far);

		newMatrix.data[0] = -2.0f * lr;
		newMatrix.data[5] = -2.0f * bt;
		newMatrix.data[10] = nf;

		newMatrix.data[12] = (left + right) * lr;
		newMatrix.data[13] = (top + bottom) * bt;
		newMatrix.data[14] = -near * nf;
		newMatrix.data[15] = 1.0f;

		/*newMatrix.data[0] = 2.0f / (right - left);
		newMatrix.data[3] = -(right + left) / (right - left);
		newMatrix.data[5] = 2.0f / (top - bottom);
		newMatrix.data[7] = -(top + bottom) / (top - bottom);
		newMatrix.data[10] = -2.0f / (far - near);
		newMatrix.data[11] = -(far + near) / (far - near);
		newMatrix.data[15] = 1.0f;*/

		return newMatrix;
	}
};

inline Matrix4x4 Mat4FromQuaternion(Quaternion& inQuat)
{
	Matrix4x4 outMat(1);

	outMat.data[0] = 1 - 2 * (inQuat.y * inQuat.y) - 2 * (inQuat.x * inQuat.z);
	outMat.data[1] = 2 * inQuat.x * inQuat.y + 2 * inQuat.w * inQuat.z;
	outMat.data[2] = 2 * (inQuat.x * inQuat.z) - 2 * (inQuat.w * inQuat.y);

	outMat.data[4] = 2 * (inQuat.x * inQuat.y) - 2 * (inQuat.w * inQuat.z);
	outMat.data[5] = 1 - 2 * (inQuat.x * inQuat.x) - 2 * (inQuat.z * inQuat.z);
	outMat.data[6] = 2 * (inQuat.y * inQuat.z) + 2 * (inQuat.w * inQuat.x);

	outMat.data[8] = 2 * (inQuat.x * inQuat.z) + 2 * (inQuat.w * inQuat.y);
	outMat.data[9] = 2 * (inQuat.y * inQuat.z) - 2 * (inQuat.w * inQuat.x);
	outMat.data[10] = 1 - 2 * (inQuat.x * inQuat.x) - 2 * (inQuat.y * inQuat.y);

	return outMat;
}