//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>
#include "Core/Assertions.h"

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

DLL_EXPORT float M_ATAN2_F(float x, float y);
DLL_EXPORT double M_ATAN2(double x, double y);

DLL_EXPORT float M_RAD2DEG_F(float angleInRadians);
DLL_EXPORT double M_RAD2DEG(double angleInRadians);

DLL_EXPORT float M_DEG2RAD_F(float angleInDegrees);
DLL_EXPORT double M_DEG2RAD(double angleInDegrees);

DLL_EXPORT float M_ABS_F(float x);
DLL_EXPORT double M_ABS(double x);

// Vector elements must be within this range to be considered 'equal'.
const float kVectorElementEpsilon = 1.192092896e-07f;

// Forward declarations
class Vector2D;
class Vector3D;
class Vector4D;

//-------------------------------------------------------------------------
// Vector2D
//-------------------------------------------------------------------------
class Vector2D
{
public:
	union
	{
		float flElements[2];
		struct
		{
			float x;
			float y;
		};
	};

	Vector2D()
		: flElements{ 0.0f }
	{
	}

	Vector2D(float inX, float inY)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
	}

	Vector2D(const Vector2D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (int idx = 0; idx < 2; idx++)
		{
			flElements[idx] = pOtherVec.flElements[idx];
		}
	}

	// --------------------
	// Addition/Subtraction
	// --------------------
	inline Vector2D Add(const Vector2D& pOtherVector) const
	{
		Vector2D outVector{ *this };
		for (int idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector2D Subtract(const Vector2D& pOtherVector) const
	{
		Vector2D outVector{ *this };
		for (int idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] -= pOtherVector.flElements[idx];
		}
		return outVector;
	}

	// --------
	// Negation
	// --------
	inline Vector2D Negate() const
	{
		Vector2D outVector{ *this };
		outVector.NegateInPlace();
		return outVector;
	}

	inline void NegateInPlace()
	{
		for (int idx = 0; idx < 2; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector2D MultiplyByScalar(float flScalar) const
	{
		Vector2D outVector{ *this };
		for (int idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(float flScalar)
	{
		for (int idx = 0; idx < 2; idx++)
		{
			flElements[idx] *= flScalar;
		}
	}

	// ----------------
	// Vector Normalize
	// ----------------
	inline Vector2D Normalize() const
	{
		Vector2D outVector{ *this };
		float flLength = outVector.Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		float flLength = Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 2; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline float Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]));
	}

	static inline float Distance(const Vector2D& pVector0, const Vector2D& pVector1)
	{
		float flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		float flYDiff = pVector1.flElements[1] - pVector0.flElements[1];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff));
	}

	// -----------------
	// Dot Product
	// -----------------
	static inline float Dot(const Vector2D& pVector0, const Vector2D& pVector1)
	{
		return (pVector0.flElements[0] * pVector1.flElements[0]) + (pVector0.flElements[1] * pVector1.flElements[1]);
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Vector2D operator+(const Vector2D& pOtherVector) const { return Add(pOtherVector); }
	inline Vector2D operator-(const Vector2D& pOtherVector) const { return Subtract(pOtherVector); }
	inline Vector2D operator-() const { return Negate(); }
	inline bool operator==(const Vector2D& pOther) const
	{
		return (M_ABS(x - pOther.x) < kVectorElementEpsilon &&
			M_ABS(y - pOther.y) < kVectorElementEpsilon);
	}
	inline Vector2D operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const float flScalar) { MultiplyByScalarInPlace(flScalar); }
};

//-------------------------------------------------------------------------
// Vector3D
//-------------------------------------------------------------------------
class Vector3D
{
public:
	union
	{
		float flElements[3];
		struct
		{
			float x;
			float y;
			float z;
		};
	};

	Vector3D()
		: flElements{ 0.0f }
	{
	}

	Vector3D(float inX, float inY, float inZ)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
		flElements[2] = inZ;
	}

	Vector3D(const Vector3D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (int idx = 0; idx < 3; idx++)
		{
			flElements[idx] = pOtherVec.flElements[idx];
		}
	}

	// --------------------
	// Addition/Subtraction
	// --------------------
	inline Vector3D Add(const Vector3D& pOtherVector) const
	{
		Vector3D outVector{ *this };
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector3D Subtract(const Vector3D& pOtherVector) const
	{
		Vector3D outVector{ *this };
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] -= pOtherVector.flElements[idx];
		}
		return outVector;
	}

	// --------
	// Negation
	// --------
	inline Vector3D Negate() const
	{
		Vector3D outVector{ *this };
		outVector.NegateInPlace();
		return outVector;
	}

	inline void NegateInPlace()
	{
		for (int idx = 0; idx < 3; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector3D MultiplyByScalar(float flScalar) const
	{
		Vector3D outVector{ *this };
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(float flScalar)
	{
		for (int idx = 0; idx < 3; idx++)
		{
			flElements[idx] *= flScalar;
		}
	}

	// ----------------------
	// Vector-Vector Division
	// ----------------------
	inline Vector3D Divide(const Vector3D& pOtherVector) const
	{
		Vector3D outVector{ *this };
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] /= pOtherVector.flElements[idx];
		}

		return outVector;
	}

	// ----------------
	// Vector Normalize
	// ----------------
	inline Vector3D Normalize() const
	{
		Vector3D outVector{ *this };
		float flLength = outVector.Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		float flLength = Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 3; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline float Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]) + (flElements[2] * flElements[2]));
	}

	static inline float Distance(const Vector3D& pVector0, const Vector3D& pVector1)
	{
		float flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		float flYDiff = pVector1.flElements[1] - pVector0.flElements[1];
		float flZDiff = pVector1.flElements[2] - pVector0.flElements[2];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff) + (flZDiff * flZDiff));
	}

	// -----------------
	// Dot/Cross Product
	// -----------------
	static inline float Dot(const Vector3D& pVector0, const Vector3D& pVector1)
	{
		return (pVector0.flElements[0] * pVector1.flElements[0]) + (pVector0.flElements[1] * pVector1.flElements[1]) + (pVector0.flElements[2] * pVector1.flElements[2]);
	}

	static inline Vector3D Cross(const Vector3D& pVector0, const Vector3D& pVector1)
	{
		Vector3D outVector{};
		outVector.flElements[0] = pVector0.flElements[1] * pVector1.flElements[2] - pVector0.flElements[2] * pVector1.flElements[1];
		outVector.flElements[1] = pVector0.flElements[2] * pVector1.flElements[0] - pVector0.flElements[0] * pVector1.flElements[2];
		outVector.flElements[2] = pVector0.flElements[0] * pVector1.flElements[1] - pVector0.flElements[1] * pVector1.flElements[0];
		return outVector;
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Vector3D operator+(const Vector3D& pOtherVector) const { return Add(pOtherVector); }
	inline Vector3D operator-(const Vector3D& pOtherVector) const { return Subtract(pOtherVector); }
	inline Vector3D operator-() const { return Negate(); }
	inline Vector3D operator+=(const Vector3D& pOtherVector) { *this = Add(pOtherVector); return *this; }
	inline Vector3D operator-=(const Vector3D& pOtherVector) { *this = Subtract(pOtherVector); return *this; }
	inline Vector3D operator/(const Vector3D& pOtherVector) const { return Divide(pOtherVector); }

	inline bool operator==(const Vector3D& pOther) const
	{
		return (M_ABS(x - pOther.x) < kVectorElementEpsilon &&
			M_ABS(y - pOther.y) < kVectorElementEpsilon &&
			M_ABS(z - pOther.z) < kVectorElementEpsilon);
	}
	inline Vector3D operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const float flScalar) { MultiplyByScalarInPlace(flScalar); }

	inline Vector4D ToVector4D() const;
};

// Perhaps the most-used alias in the entire codebase.
typedef Vector3D Vector;

//-------------------------------------------------------------------------
// Vector4D
//-------------------------------------------------------------------------
class Vector4D
{
public:
	union
	{
		float flElements[4];
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};

	Vector4D()
		: flElements{ 0.0f }
	{
	}

	Vector4D(float inX, float inY, float inZ, float inW)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
		flElements[2] = inZ;
		flElements[3] = inW;
	}

	Vector4D(const Vector3D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (int idx = 0; idx < 4; idx++)
		{
			flElements[idx] = pOtherVec.flElements[idx];
		}
	}

	// --------------------
	// Addition/Subtraction
	// --------------------
	inline Vector4D Add(const Vector3D& pOtherVector) const
	{
		Vector4D outVector{ *this };
		for (int idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector4D Subtract(const Vector3D& pOtherVector) const
	{
		Vector4D outVector{ *this };
		for (int idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] -= pOtherVector.flElements[idx];
		}
		return outVector;
	}

	// --------
	// Negation
	// --------
	inline Vector4D Negate() const
	{
		Vector4D outVector{ *this };
		outVector.NegateInPlace();
		return outVector;
	}

	inline void NegateInPlace()
	{
		for (int idx = 0; idx < 4; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector4D MultiplyByScalar(float flScalar) const
	{
		Vector4D outVector{ *this };
		for (int idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(float flScalar)
	{
		for (int idx = 0; idx < 4; idx++)
		{
			flElements[idx] *= flScalar;
		}
	}

	// ----------------
	// Vector Normalize
	// ----------------
	inline Vector4D Normalize() const
	{
		Vector4D outVector{ *this };
		float flLength = outVector.Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		float flLength = Length();
		CASSERTMSG(flLength > 0.0001f, "Divide by 0 incoming!");
		for (int idx = 0; idx < 4; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline float Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]) + (flElements[2] * flElements[2]) + (flElements[3] * flElements[3]));
	}

	static inline float Distance(const Vector4D& pVector0, const Vector4D& pVector1)
	{
		float flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		float flYDiff = pVector1.flElements[1] - pVector0.flElements[1];
		float flZDiff = pVector1.flElements[2] - pVector0.flElements[2];
		float flWDiff = pVector1.flElements[3] - pVector0.flElements[3];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff) + (flZDiff * flZDiff) + (flWDiff * flWDiff));
	}

	// -----------------
	// Dot Product
	// -----------------
	static inline float Dot(const Vector4D& pVector0, const Vector4D& pVector1)
	{
		return (pVector0.flElements[0] * pVector1.flElements[0]) + (pVector0.flElements[1] * pVector1.flElements[1]) + (pVector0.flElements[2] * pVector1.flElements[2]) + (pVector0.flElements[3] * pVector1.flElements[3]);
	}

	inline Vector3D AsVector3D() const
	{
		return Vector3D{ flElements[0], flElements[1], flElements[2] };
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Vector4D operator+(const Vector3D& pOtherVector) const { return Add(pOtherVector); }
	inline Vector4D operator-(const Vector3D& pOtherVector) const { return Subtract(pOtherVector); }
	inline Vector4D operator-() const { return Negate(); }
	inline Vector4D operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const float flScalar) { MultiplyByScalarInPlace(flScalar); }
};

//-------------------------------------------------------------------------
// Purpose: Utility to convert a 3D Vector to a 4D Vector (with a 1.0 w component).
//-------------------------------------------------------------------------
Vector4D Vector3D::ToVector4D() const
{
	 return Vector4D{ x, y, z, 1.0f }; 
}

//-------------------------------------------------------------------------
// Quaternion
//-------------------------------------------------------------------------
class Quaternion
{
public:
	float x;
	float y;
	float z;
	float w;
	Quaternion()
		: x{ 0 }
		, y{ 0 }
		, z{ 0 }
		, w{ 0 } {
	}

	Quaternion(float x_, float y_, float z_, float w_)
		: x{ x_ }
		, y{ y_ }
		, z{ z_ }
		, w{ w_ } {
	}

	Quaternion(Quaternion& pOtherQuat)
		: x{ pOtherQuat.x }
		, y{ pOtherQuat.y }
		, z{ pOtherQuat.z }
		, w{ pOtherQuat.w } {
	}
};

//-------------------------------------------------------------------------
// Matrix4x4
//-------------------------------------------------------------------------
class Matrix4x4
{
public:
	float flData[16];
	Matrix4x4()
		: flData{ 0.0f }
	{
	}

	// Creates an identity matrix scaled by flIdentityValue
	// (I.E. flIdentityValue == 1 will create an identity matrix).
	Matrix4x4(float flIdentityValue)
		: flData{ 0.0f }
	{
		flData[0] = flIdentityValue;
		flData[5] = flIdentityValue;
		flData[10] = flIdentityValue;
		flData[15] = flIdentityValue;
	}

	Matrix4x4(const Matrix4x4& pOtherMatrix)
		: flData{ 0.0f }
	{
		for (int idx = 0; idx < 16; idx++)
		{
			flData[idx] = pOtherMatrix.flData[idx];
		}
	}

	// ----------------------------
	// Matrix-Scalar Multiplication
	// ----------------------------
	inline Matrix4x4 MultiplyByScalar(const float flScalar) const
	{
		Matrix4x4 outMatrix{ *this };
		for (int idx = 0; idx < 16; idx++)
		{
			outMatrix.flData[idx] *= flScalar;
		}
		return outMatrix;
	}

	// ----------------------------
	// Matrix-Vector Multiplication
	// ----------------------------
	inline Vector3D MultiplyByVector(const Vector& pVector) const
	{
		Vector3D outVector{};
		outVector.x = (pVector.x * flData[0]) + (pVector.y * flData[1]) + (pVector.z * flData[2]) + (flData[12]);
		outVector.y = (pVector.x * flData[4]) + (pVector.y * flData[5]) + (pVector.z * flData[6]) + (flData[13]);
		outVector.z = (pVector.x * flData[8]) + (pVector.y * flData[9]) + (pVector.z * flData[10] + (flData[14]));
		return outVector;
	}

	inline Vector4D MultiplyByVector(const Vector4D& pVector) const
	{
		Vector4D outVector{};
		outVector.x = (pVector.x * flData[0]) + (pVector.y * flData[4]) + (pVector.z * flData[8]) + (pVector.w * flData[12]);
		outVector.y = (pVector.x * flData[1]) + (pVector.y * flData[5]) + (pVector.z * flData[9]) + (pVector.w * flData[13]);
		outVector.z = (pVector.x * flData[2]) + (pVector.y * flData[6]) + (pVector.z * flData[10]) + (pVector.w * flData[14]);
		outVector.w = (pVector.x * flData[3]) + (pVector.y * flData[7]) + (pVector.z * flData[11]) + (pVector.w * flData[15]);
		return outVector;
	}

	// ----------------------------
	// Matrix-Matrix Multiplication
	// ----------------------------
	inline Matrix4x4 MultiplyByMatrix(const Matrix4x4& pOtherMatrix) const
	{
		Matrix4x4 outMatrix{};
		const float* mat1Ptr = &flData[0];
		const float* mat2Ptr = &pOtherMatrix.flData[0];

		for (int firstMat = 0; firstMat < 4; firstMat++)
		{
			for (int secondMat = 0; secondMat < 4; secondMat++)
			{
				outMatrix.flData[(firstMat * 4) + secondMat] =
					(mat1Ptr[(firstMat * 4)] * mat2Ptr[secondMat])
					+ (mat1Ptr[(firstMat * 4) + 1] * mat2Ptr[secondMat + 4])
					+ (mat1Ptr[(firstMat * 4) + 2] * mat2Ptr[secondMat + 8])
					+ (mat1Ptr[(firstMat * 4) + 3] * mat2Ptr[secondMat + 12]);
			}
		}

		return outMatrix;
	}

	// --------------
	// Matrix Inverse
	// --------------
	inline Matrix4x4 Inverse() const
	{
		Matrix4x4 outMatrix{ *this };

		float flT1 = ((flData[10] * flData[15]) - (flData[11] * flData[14]));
		float flT2 = ((flData[9] * flData[15]) - (flData[11] * flData[13]));
		float flT3 = ((flData[9] * flData[14]) - (flData[10] * flData[13]));
		float flT4 = ((flData[10] * flData[15]) - (flData[11] * flData[14]));
		float flT5 = ((flData[8] * flData[15]) - (flData[11] * flData[12]));
		float flT6 = ((flData[8] * flData[14]) - (flData[10] * flData[12]));
		float flT7 = ((flData[9] * flData[15]) - (flData[11] * flData[13]));
		float flT8 = ((flData[8] * flData[15]) - (flData[11] * flData[12]));
		float flT9 = ((flData[8] * flData[13]) - (flData[9] * flData[12]));
		float flT10 = ((flData[9] * flData[14]) - (flData[10] * flData[13]));
		float flT11 = ((flData[8] * flData[14]) - (flData[10] * flData[12]));
		float flT12 = ((flData[8] * flData[13]) - (flData[9] * flData[12]));
		float flT13 = ((flData[6] * flData[15]) - (flData[7] * flData[14]));
		float flT14 = ((flData[5] * flData[15]) - (flData[7] * flData[13]));
		float flT15 = ((flData[5] * flData[14]) - (flData[6] * flData[13]));
		float flT16 = ((flData[4] * flData[15]) - (flData[7] * flData[12]));
		float flT17 = ((flData[4] * flData[14]) - (flData[6] * flData[12]));
		float flT18 = ((flData[4] * flData[13]) - (flData[5] * flData[12]));
		float flT19 = ((flData[6] * flData[11]) - (flData[7] * flData[10]));
		float flT20 = ((flData[5] * flData[11]) - (flData[7] * flData[9]));
		float flT21 = ((flData[5] * flData[10]) - (flData[6] * flData[9]));
		float flT22 = ((flData[4] * flData[11]) - (flData[7] * flData[8]));
		float flT23 = ((flData[4] * flData[10]) - (flData[6] * flData[8]));
		float flT24 = ((flData[4] * flData[9]) - (flData[5] * flData[8]));

		// Compute the determinant.
		float flDeterminant = (flData[0] * (flData[5] * flT1 - flData[6] * flT2 + flData[7] * flT3))
							- (flData[1] * (flData[4] * flT4 - flData[6] * flT5 + flData[7] * flT6))
							+ (flData[2] * (flData[4] * flT7 - flData[5] * flT8 + flData[7] * flT9)
							- (flData[3] * (flData[4] * flT10 - flData[5] * flT11 + flData[6] * flT12)));

		// If the determinant is 0, then we cannot find an inverse matrix with this method, so return an identity matrix to be safe.
		if (flDeterminant > 0.0001f && flDeterminant < 0.0001f)
			return Matrix4x4::Matrix4x4(1.0f);

		// Create the temp co-factor matrix.
		float flTempMatrix[16]{ 0.0f };
		flTempMatrix[0] = (flData[5] * flT1 - flData[6] * flT2 + flData[7] * flT3);
		flTempMatrix[1] = -1.0f * (flData[4] * flT4 - flData[6] * flT5 + flData[7] * flT6);
		flTempMatrix[2] = (flData[4] * flT7 - flData[5] * flT8 + flData[7] * flT9);
		flTempMatrix[3] = -1.0f * (flData[4] * flT10 - flData[5] * flT11 + flData[6] * flT12);
		flTempMatrix[4] = -1.0f * (flData[1] * flT1 - flData[2] * flT2 + flData[3] * flT3);
		flTempMatrix[5] = (flData[0] * flT4 - flData[2] * flT5 + flData[3] * flT6);
		flTempMatrix[6] = -1.0f * (flData[0] * flT2 - flData[1] * flT8 + flData[3] * flT9);
		flTempMatrix[7] = (flData[0] * flT10 - flData[1] * flT6 + flData[2] * flT9);
		flTempMatrix[8] = (flData[1] * flT13 - flData[2] * flT14 + flData[3] * flT15);
		flTempMatrix[9] = -1.0f * (flData[0] * flT13 - flData[2] * flT16 + flData[3] * flT17);
		flTempMatrix[10] = (flData[0] * flT14 - flData[1] * flT16 + flData[3] * flT18);
		flTempMatrix[11] = -1.0f * (flData[0] * flT15 - flData[1] * flT17 + flData[2] * flT18);
		flTempMatrix[12] = -1.0f * (flData[1] * flT19 - flData[2] * flT20 + flData[3] * flT21);
		flTempMatrix[13] = (flData[0] * flT19 - flData[2] * flT22 + flData[3] * flT23);
		flTempMatrix[14] = -1.0f * (flData[0] * flT20 - flData[1] * flT22 + flData[3] * flT24);
		flTempMatrix[15] = (flData[0] * flT21 - flData[1] * flT23 + flData[2] * flT24);

		// Transpose the co-factor matrix and write it to the output.
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				outMatrix.flData[(i * 4) + j] = flTempMatrix[(j * 4) + i];
			}
		}

		// Finally, divide the new adjunct matrix by the determinant.
		outMatrix = outMatrix * (1.0f / flDeterminant);

		return outMatrix;
	}

	// ---------
	// Translate
	// ---------
	static inline Matrix4x4 Translate(const Matrix4x4& pInMatrix, const Vector& pInVector)
	{
		Matrix4x4 matTranslate{ pInMatrix };
		matTranslate.flData[12] += pInVector.x;
		matTranslate.flData[13] += pInVector.y;
		matTranslate.flData[14] += pInVector.z;
		return matTranslate;
	}

	// -----
	// Scale
	// -----
	static inline Matrix4x4 Scale(const Matrix4x4& pInMatrix, const Vector& pInScale)
	{
		Matrix4x4 matScale{ pInMatrix };
		matScale.flData[0] *= pInScale.x;
		matScale.flData[5] *= pInScale.y;
		matScale.flData[10] *= pInScale.z;
		return matScale;
	}

	// ------
	// Rotate
	// ------
	static inline Matrix4x4 RotateAboutXAxis(const Matrix4x4& pInMatrix, const float flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[5] = M_COS_F(flAngleInRadians);
		matRotate.flData[6] = M_SIN_F(flAngleInRadians);
		matRotate.flData[9] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[10] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutYAxis(const Matrix4x4& pInMatrix, const float flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[0] = M_COS_F(flAngleInRadians);
		matRotate.flData[2] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[8] = M_SIN_F(flAngleInRadians);
		matRotate.flData[10] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutZAxis(const Matrix4x4& pInMatrix, const float flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[0] = M_COS_F(flAngleInRadians);
		matRotate.flData[1] = M_SIN_F(flAngleInRadians);
		matRotate.flData[4] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[5] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutAxisAngles(const Matrix4x4& pInMatrix, const Vector& pInRotationAxis, const float flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[0] = M_COS_F(flAngleInRadians) + ((pInRotationAxis.x * pInRotationAxis.x)) * (1 - M_COS_F(flAngleInRadians));
		matRotate.flData[1] = (pInRotationAxis.y * pInRotationAxis.x * (1 - M_COS_F(flAngleInRadians))) + (pInRotationAxis.z * M_SIN_F(flAngleInRadians));
		matRotate.flData[2] = (pInRotationAxis.z * pInRotationAxis.x * (1 - M_COS_F(flAngleInRadians))) - (pInRotationAxis.y * M_SIN_F(flAngleInRadians));
		matRotate.flData[4] = pInRotationAxis.x * pInRotationAxis.y * (1 - M_COS_F(flAngleInRadians)) - pInRotationAxis.z * M_SIN_F(flAngleInRadians);
		matRotate.flData[5] = M_COS_F(flAngleInRadians) + (pInRotationAxis.y * pInRotationAxis.y * (1 - M_COS_F(flAngleInRadians)));
		matRotate.flData[6] = (pInRotationAxis.z * pInRotationAxis.y * (1 - M_COS_F(flAngleInRadians))) + (pInRotationAxis.x * M_SIN_F(flAngleInRadians));
		matRotate.flData[8] = (pInRotationAxis.x * pInRotationAxis.z * (1 - M_COS_F(flAngleInRadians))) + (pInRotationAxis.y * M_SIN_F(flAngleInRadians));
		matRotate.flData[9] = (pInRotationAxis.y * pInRotationAxis.z * (1 - M_COS_F(flAngleInRadians))) - (pInRotationAxis.x * M_SIN_F(flAngleInRadians));
		matRotate.flData[10] = (M_COS_F(flAngleInRadians) + (pInRotationAxis.z * pInRotationAxis.z) * (1 - M_COS_F(flAngleInRadians)));
		return matRotate;
	}

	// ------
	// LookAt
	// ------
	static inline Matrix4x4 LookAt(Vector& vecPosition, Vector& vecTarget, Vector& pVecUp)
	{
		// Compute the camera's local space basis.
		Vector3D vecForward = (vecTarget - vecPosition).Normalize();
		Vector3D vecRight = (Vector3D::Cross(vecForward, pVecUp)).Normalize();
		Vector3D vecUp = (Vector3D::Cross(vecRight, vecForward)).Normalize();

		Matrix4x4 viewMat{ 1.0f };

		viewMat.flData[0] = vecRight.x;
		viewMat.flData[1] = vecUp.x;
		viewMat.flData[2] = -vecForward.x;
		viewMat.flData[4] = vecRight.y;
		viewMat.flData[5] = vecUp.y;
		viewMat.flData[6] = -vecForward.y;
		viewMat.flData[8] = vecRight.z;
		viewMat.flData[9] = vecUp.z;
		viewMat.flData[10] = -vecForward.z;
		viewMat.flData[12] = -Vector3D::Dot(vecPosition, vecRight);
		viewMat.flData[13] = -Vector3D::Dot(vecPosition, vecUp);
		viewMat.flData[14] = Vector3D::Dot(vecPosition, vecForward);

		return viewMat;
	}

	// ----------
	// Projection
	// ----------
	static inline Matrix4x4 Perspective(float flFOV, float flAspectRatio, float flNearZ, float flFarZ)
	{
		Matrix4x4 projMat{ 1.0f };

		float flHalfTanFOV = M_TAN_F(flFOV * 0.5f);
		projMat.flData[0] = 1.0f / (flAspectRatio * flHalfTanFOV);
		projMat.flData[5] = 1.0f / flHalfTanFOV;
		projMat.flData[10] = -((flFarZ + flNearZ) / (flFarZ - flNearZ));
		projMat.flData[11] = -1.0f;
		projMat.flData[14] = -((2.0f * flFarZ * flNearZ) / (flFarZ - flNearZ));

		return projMat;
	}

	static inline Matrix4x4 Orthographic(float flLeft, float flRight, float flBottom, float flTop, float flNear, float flFar)
	{
		Matrix4x4 projMat{ 1.0f };

		/*float lr = 1.0f / (flLeft - flRight);
		float bt = 1.0f / (flBottom - flTop);
		float nf = 1.0f / (flNear - flFar);

		projMat.flData[0] = -2.0f * lr;
		projMat.flData[5] = -2.0f * bt;
		projMat.flData[10] = nf;

		projMat.flData[12] = (flLeft + flRight) * lr;
		projMat.flData[13] = (flTop + flBottom) * bt;
		projMat.flData[14] = -flNear * nf;
		projMat.flData[15] = 1.0f;*/

		projMat.flData[0] = 2.0f / (flRight - flLeft);
		projMat.flData[5] = 2.0f / (flTop - flBottom);
		projMat.flData[10] = -2.0f / (flFar - flNear);
		projMat.flData[12] = -(flRight + flLeft) / (flRight - flLeft);
		projMat.flData[13] = -(flTop + flBottom) / (flTop - flBottom);
		projMat.flData[14] = -(flFar + flNear) / (flFar - flNear);
		projMat.flData[15] = 1.0f;

		return projMat;
	}

	// --------------------
	// Matrix -> Quaternion
	// --------------------
	static inline Matrix4x4 Mat4FromQuaternion(class Quaternion& inQuat)
	{
		Matrix4x4 outMat{ 1.0f };

		outMat.flData[0] = 1 - 2 * (inQuat.y * inQuat.y) - 2 * (inQuat.x * inQuat.z);
		outMat.flData[1] = 2 * inQuat.x * inQuat.y + 2 * inQuat.w * inQuat.z;
		outMat.flData[2] = 2 * (inQuat.x * inQuat.z) - 2 * (inQuat.w * inQuat.y);

		outMat.flData[4] = 2 * (inQuat.x * inQuat.y) - 2 * (inQuat.w * inQuat.z);
		outMat.flData[5] = 1 - 2 * (inQuat.x * inQuat.x) - 2 * (inQuat.z * inQuat.z);
		outMat.flData[6] = 2 * (inQuat.y * inQuat.z) + 2 * (inQuat.w * inQuat.x);

		outMat.flData[8] = 2 * (inQuat.x * inQuat.z) + 2 * (inQuat.w * inQuat.y);
		outMat.flData[9] = 2 * (inQuat.y * inQuat.z) - 2 * (inQuat.w * inQuat.x);
		outMat.flData[10] = 1 - 2 * (inQuat.x * inQuat.x) - 2 * (inQuat.y * inQuat.y);

		return outMat;
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Matrix4x4 operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
	inline Vector3D operator*(const Vector3D& pInVector) const { return (MultiplyByVector(pInVector.ToVector4D())).AsVector3D(); }	// NOTE: This performs vector PRE-Multiplication, not POST! Post is undefined.
	inline Vector4D operator*(const Vector4D& pInVector) const { return MultiplyByVector(pInVector); }								// NOTE: This performs vector PRE-Multiplication, not POST! Post is undefined.
	inline Matrix4x4 operator*(const Matrix4x4& pMatrix) const { return MultiplyByMatrix(pMatrix); }
};

// Quaternion operations
inline Quaternion QuaternionFromAxisAngle(Vector3D& vecAxisOfRotation, float flAngle)
{
	Quaternion outQuat{};
	outQuat.x = M_SIN_F(flAngle / 2.0f) * vecAxisOfRotation.x;
	outQuat.y = M_SIN_F(flAngle / 2.0f) * vecAxisOfRotation.y;
	outQuat.z = M_SIN_F(flAngle / 2.0f) * vecAxisOfRotation.z;
	outQuat.w = M_COS_F(flAngle / 2.0f);
	return outQuat;
}

// NOTE: This expects the euler angles to be in degrees and this method will convert to radians.
inline Quaternion QuaternionFromEulerAngles(Vector3D& vecEulerAngles)
{
	Quaternion outQuat{};

	float flSinX = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.x) / 2);
	float flCosX = M_COS_F(M_DEG2RAD_F(vecEulerAngles.x) / 2);
	float flSinY = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.y) / 2);
	float flCosY = M_COS_F(M_DEG2RAD_F(vecEulerAngles.y) / 2);
	float flSinZ = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.z) / 2);
	float flCosZ = M_COS_F(M_DEG2RAD_F(vecEulerAngles.z) / 2);

	outQuat.x = flCosX * flSinY * flCosZ + flSinX * flCosY * flSinZ;
	outQuat.y = flSinX * flCosY * flCosZ - flCosX * flSinX * flSinZ;
	outQuat.z = flCosX * flCosY * flSinZ - flSinX * flSinY * flCosZ;
	outQuat.w = flCosX * flCosY * flCosZ + flSinX * flCosX * flSinZ;

	return outQuat;
}
