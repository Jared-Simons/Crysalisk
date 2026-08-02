//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>

const f64 kPI = 3.14159265359;
const f32 kPI_F = 3.14159265359f;

DLL_EXPORT f32 M_SQRT_F(f32 x);
DLL_EXPORT f64 M_SQRT(f64 x);

DLL_EXPORT f32 M_SIN_F(f32 x);
DLL_EXPORT f64 M_SIN(f64 x);

DLL_EXPORT f32 M_COS_F(f32 x);
DLL_EXPORT f64 M_COS(f64 x);

DLL_EXPORT f32 M_TAN_F(f32 x);
DLL_EXPORT f64 M_TAN(f64 x);

DLL_EXPORT f32 M_ATAN2_F(f32 x, f32 y);
DLL_EXPORT f64 M_ATAN2(f64 x, f64 y);

DLL_EXPORT f32 M_RAD2DEG_F(f32 angleInRadians);
DLL_EXPORT f64 M_RAD2DEG(f64 angleInRadians);

DLL_EXPORT f32 M_DEG2RAD_F(f32 angleInDegrees);
DLL_EXPORT f64 M_DEG2RAD(f64 angleInDegrees);

DLL_EXPORT f32 M_ABS_F(f32 x);
DLL_EXPORT f64 M_ABS(f64 x);

// Vector elements must be within this range to be considered 'equal'.
const f32 kVectorElementEpsilon = 1.192092896e-07f;

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
		f32 flElements[2];
		struct
		{
			f32 x;
			f32 y;
		};
	};

	inline Vector2D()
		: flElements{ 0.0f }
	{
	}

	inline Vector2D(f32 inX, f32 inY)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
	}

	inline Vector2D(const Vector2D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (i32 idx = 0; idx < 2; idx++)
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
		for (i32 idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector2D Subtract(const Vector2D& pOtherVector) const
	{
		Vector2D outVector{ *this };
		for (i32 idx = 0; idx < 2; idx++)
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
		for (i32 idx = 0; idx < 2; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector2D MultiplyByScalar(f32 flScalar) const
	{
		Vector2D outVector{ *this };
		for (i32 idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(f32 flScalar)
	{
		for (i32 idx = 0; idx < 2; idx++)
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
		f32 flLength = outVector.Length();
		for (i32 idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		f32 flLength = Length();
		for (i32 idx = 0; idx < 2; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline f32 Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]));
	}

	static inline f32 Distance(const Vector2D& pVector0, const Vector2D& pVector1)
	{
		f32 flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		f32 flYDiff = pVector1.flElements[1] - pVector0.flElements[1];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff));
	}

	// -----------------
	// Dot Product
	// -----------------
	static inline f32 Dot(const Vector2D& pVector0, const Vector2D& pVector1)
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
	inline Vector2D operator*(const f32 flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const f32 flScalar) { MultiplyByScalarInPlace(flScalar); }
};

//-------------------------------------------------------------------------
// Vector3D
//-------------------------------------------------------------------------
class Vector3D
{
public:
	union
	{
		f32 flElements[3];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
		};
	};

	inline Vector3D()
		: flElements{ 0.0f }
	{
	}

	inline Vector3D(f32 inX, f32 inY, f32 inZ)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
		flElements[2] = inZ;
	}

	inline Vector3D(const Vector3D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (i32 idx = 0; idx < 3; idx++)
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
		for (i32 idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector3D Subtract(const Vector3D& pOtherVector) const
	{
		Vector3D outVector{ *this };
		for (i32 idx = 0; idx < 3; idx++)
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
		for (i32 idx = 0; idx < 3; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector3D MultiplyByScalar(f32 flScalar) const
	{
		Vector3D outVector{ *this };
		for (i32 idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(f32 flScalar)
	{
		for (i32 idx = 0; idx < 3; idx++)
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
		for (i32 idx = 0; idx < 3; idx++)
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
		f32 flLength = outVector.Length();
		for (i32 idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		f32 flLength = Length();
		for (i32 idx = 0; idx < 3; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline f32 Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]) + (flElements[2] * flElements[2]));
	}

	static inline f32 Distance(const Vector3D& pVector0, const Vector3D& pVector1)
	{
		f32 flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		f32 flYDiff = pVector1.flElements[1] - pVector0.flElements[1];
		f32 flZDiff = pVector1.flElements[2] - pVector0.flElements[2];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff) + (flZDiff * flZDiff));
	}

	// -----------------
	// Dot/Cross Product
	// -----------------
	static inline f32 Dot(const Vector3D& pVector0, const Vector3D& pVector1)
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

	inline bool operator!=(const Vector3D& pOther) const
	{
		bool result = (*this == pOther);
		return !result;
	}

	inline Vector3D operator*(const f32 flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const f32 flScalar) { MultiplyByScalarInPlace(flScalar); }

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
		f32 flElements[4];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
			f32 w;
		};
	};

	inline Vector4D()
		: flElements{ 0.0f }
	{
	}

	inline Vector4D(f32 inX, f32 inY, f32 inZ, f32 inW)
		: flElements{ 0.0f }
	{
		flElements[0] = inX;
		flElements[1] = inY;
		flElements[2] = inZ;
		flElements[3] = inW;
	}

	inline Vector4D(const Vector3D& pOtherVec)
		: flElements{ 0.0f }
	{
		for (i32 idx = 0; idx < 4; idx++)
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
		for (i32 idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] += pOtherVector.flElements[idx];
		}
		return outVector;
	}

	inline Vector4D Subtract(const Vector3D& pOtherVector) const
	{
		Vector4D outVector{ *this };
		for (i32 idx = 0; idx < 4; idx++)
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
		for (i32 idx = 0; idx < 4; idx++)
		{
			flElements[idx] = -flElements[idx];
		}
	}

	// ----------------------------
	// Vector-Scalar Multiplication
	// ----------------------------
	inline Vector4D MultiplyByScalar(f32 flScalar) const
	{
		Vector4D outVector{ *this };
		for (i32 idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] = flElements[idx] * flScalar;
		}
		return outVector;
	}

	inline void MultiplyByScalarInPlace(f32 flScalar)
	{
		for (i32 idx = 0; idx < 4; idx++)
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
		f32 flLength = outVector.Length();
		for (i32 idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		f32 flLength = Length();
		for (i32 idx = 0; idx < 4; idx++)
		{
			flElements[idx] /= flLength;
		}
	}

	// ----------------
	// Vector Magnitude
	// ----------------
	inline f32 Length() const
	{
		return M_SQRT_F((flElements[0] * flElements[0]) + (flElements[1] * flElements[1]) + (flElements[2] * flElements[2]) + (flElements[3] * flElements[3]));
	}

	static inline f32 Distance(const Vector4D& pVector0, const Vector4D& pVector1)
	{
		f32 flXDiff = pVector1.flElements[0] - pVector0.flElements[0];
		f32 flYDiff = pVector1.flElements[1] - pVector0.flElements[1];
		f32 flZDiff = pVector1.flElements[2] - pVector0.flElements[2];
		f32 flWDiff = pVector1.flElements[3] - pVector0.flElements[3];

		return M_SQRT_F((flXDiff * flXDiff) + (flYDiff * flYDiff) + (flZDiff * flZDiff) + (flWDiff * flWDiff));
	}

	// -----------------
	// Dot Product
	// -----------------
	static inline f32 Dot(const Vector4D& pVector0, const Vector4D& pVector1)
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
	inline Vector4D operator*(const f32 flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const f32 flScalar) { MultiplyByScalarInPlace(flScalar); }
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
	f32 x;
	f32 y;
	f32 z;
	f32 w;
	inline Quaternion()
		: x{ 0 }
		, y{ 0 }
		, z{ 0 }
		, w{ 0 } {
	}

	inline Quaternion(f32 x_, f32 y_, f32 z_, f32 w_)
		: x{ x_ }
		, y{ y_ }
		, z{ z_ }
		, w{ w_ } {
	}

	inline Quaternion(Quaternion& pOtherQuat)
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
	f32 flData[16];
	inline Matrix4x4()
		: flData{ 0.0f }
	{
	}

	// Creates an identity matrix scaled by flIdentityValue
	// (I.E. flIdentityValue == 1 will create an identity matrix).
	inline Matrix4x4(f32 flIdentityValue)
		: flData{ 0.0f }
	{
		flData[0] = flIdentityValue;
		flData[5] = flIdentityValue;
		flData[10] = flIdentityValue;
		flData[15] = flIdentityValue;
	}

	inline Matrix4x4(const Matrix4x4& pOtherMatrix)
		: flData{ 0.0f }
	{
		for (i32 idx = 0; idx < 16; idx++)
		{
			flData[idx] = pOtherMatrix.flData[idx];
		}
	}

	// ----------------------------
	// Matrix-Scalar Multiplication
	// ----------------------------
	inline Matrix4x4 MultiplyByScalar(const f32 flScalar) const
	{
		Matrix4x4 outMatrix{ *this };
		for (i32 idx = 0; idx < 16; idx++)
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
		const f32* mat1Ptr = &flData[0];
		const f32* mat2Ptr = &pOtherMatrix.flData[0];

		for (i32 firstMat = 0; firstMat < 4; firstMat++)
		{
			for (i32 secondMat = 0; secondMat < 4; secondMat++)
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

		f32 flT1 = ((flData[10] * flData[15]) - (flData[11] * flData[14]));
		f32 flT2 = ((flData[9] * flData[15]) - (flData[11] * flData[13]));
		f32 flT3 = ((flData[9] * flData[14]) - (flData[10] * flData[13]));
		f32 flT4 = ((flData[10] * flData[15]) - (flData[11] * flData[14]));
		f32 flT5 = ((flData[8] * flData[15]) - (flData[11] * flData[12]));
		f32 flT6 = ((flData[8] * flData[14]) - (flData[10] * flData[12]));
		f32 flT7 = ((flData[9] * flData[15]) - (flData[11] * flData[13]));
		f32 flT8 = ((flData[8] * flData[15]) - (flData[11] * flData[12]));
		f32 flT9 = ((flData[8] * flData[13]) - (flData[9] * flData[12]));
		f32 flT10 = ((flData[9] * flData[14]) - (flData[10] * flData[13]));
		f32 flT11 = ((flData[8] * flData[14]) - (flData[10] * flData[12]));
		f32 flT12 = ((flData[8] * flData[13]) - (flData[9] * flData[12]));
		f32 flT13 = ((flData[6] * flData[15]) - (flData[7] * flData[14]));
		f32 flT14 = ((flData[5] * flData[15]) - (flData[7] * flData[13]));
		f32 flT15 = ((flData[5] * flData[14]) - (flData[6] * flData[13]));
		f32 flT16 = ((flData[4] * flData[15]) - (flData[7] * flData[12]));
		f32 flT17 = ((flData[4] * flData[14]) - (flData[6] * flData[12]));
		f32 flT18 = ((flData[4] * flData[13]) - (flData[5] * flData[12]));
		f32 flT19 = ((flData[6] * flData[11]) - (flData[7] * flData[10]));
		f32 flT20 = ((flData[5] * flData[11]) - (flData[7] * flData[9]));
		f32 flT21 = ((flData[5] * flData[10]) - (flData[6] * flData[9]));
		f32 flT22 = ((flData[4] * flData[11]) - (flData[7] * flData[8]));
		f32 flT23 = ((flData[4] * flData[10]) - (flData[6] * flData[8]));
		f32 flT24 = ((flData[4] * flData[9]) - (flData[5] * flData[8]));

		// Compute the determinant.
		f32 flDeterminant = (flData[0] * (flData[5] * flT1 - flData[6] * flT2 + flData[7] * flT3))
							- (flData[1] * (flData[4] * flT4 - flData[6] * flT5 + flData[7] * flT6))
							+ (flData[2] * (flData[4] * flT7 - flData[5] * flT8 + flData[7] * flT9)
							- (flData[3] * (flData[4] * flT10 - flData[5] * flT11 + flData[6] * flT12)));

		// If the determinant is 0, then we cannot find an inverse matrix with this method, so return an identity matrix to be safe.
		if (flDeterminant > 0.0001f && flDeterminant < 0.0001f)
			return Matrix4x4::Matrix4x4(1.0f);

		// Create the temp co-factor matrix.
		f32 flTempMatrix[16]{ 0.0f };
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
		for (i32 i = 0; i < 4; i++)
		{
			for (i32 j = 0; j < 4; j++)
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
	static inline Matrix4x4 RotateAboutXAxis(const Matrix4x4& pInMatrix, const f32 flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[5] = M_COS_F(flAngleInRadians);
		matRotate.flData[6] = M_SIN_F(flAngleInRadians);
		matRotate.flData[9] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[10] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutYAxis(const Matrix4x4& pInMatrix, const f32 flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[0] = M_COS_F(flAngleInRadians);
		matRotate.flData[2] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[8] = M_SIN_F(flAngleInRadians);
		matRotate.flData[10] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutZAxis(const Matrix4x4& pInMatrix, const f32 flAngleInRadians)
	{
		Matrix4x4 matRotate{ pInMatrix };
		matRotate.flData[0] = M_COS_F(flAngleInRadians);
		matRotate.flData[1] = M_SIN_F(flAngleInRadians);
		matRotate.flData[4] = -M_SIN_F(flAngleInRadians);
		matRotate.flData[5] = M_COS_F(flAngleInRadians);
		return matRotate;
	}

	static inline Matrix4x4 RotateAboutAxisAngles(const Matrix4x4& pInMatrix, const Vector& pInRotationAxis, const f32 flAngleInRadians)
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
	static inline Matrix4x4 Perspective(f32 flFOV, f32 flAspectRatio, f32 flNearZ, f32 flFarZ)
	{
		Matrix4x4 projMat{ 1.0f };

		f32 flHalfTanFOV = M_TAN_F(flFOV * 0.5f);
		projMat.flData[0] = 1.0f / (flAspectRatio * flHalfTanFOV);
		projMat.flData[5] = 1.0f / flHalfTanFOV;
		projMat.flData[10] = -((flFarZ + flNearZ) / (flFarZ - flNearZ));
		projMat.flData[11] = -1.0f;
		projMat.flData[14] = -((2.0f * flFarZ * flNearZ) / (flFarZ - flNearZ));

		return projMat;
	}

	static inline Matrix4x4 Orthographic(f32 flLeft, f32 flRight, f32 flBottom, f32 flTop, f32 flNear, f32 flFar)
	{
		Matrix4x4 projMat{ 1.0f };

		f32 flLR = 1.0f / (flLeft - flRight);
		f32 flBT = 1.0f / (flBottom - flTop);
		f32 flNF = 1.0f / (flNear - flFar);

		projMat.flData[0] = -2.0f * flLR;
		projMat.flData[5] = -2.0f * flBT;
		projMat.flData[10] = flNF;

		projMat.flData[12] = (flLeft + flRight) * flLR;
		projMat.flData[13] = (flTop + flBottom) * flBT;
		projMat.flData[14] = -flNear * flNF;

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

	inline Vector GetTranslation() const
	{
		return Vector{ flData[12], flData[13], flData[14] };
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Matrix4x4 operator*(const f32 flScalar) const { return MultiplyByScalar(flScalar); }
	inline Vector3D operator*(const Vector3D& pInVector) const { return (MultiplyByVector(pInVector.ToVector4D())).AsVector3D(); }	// NOTE: This performs vector PRE-Multiplication, not POST! Post is undefined.
	inline Vector4D operator*(const Vector4D& pInVector) const { return MultiplyByVector(pInVector); }								// NOTE: This performs vector PRE-Multiplication, not POST! Post is undefined.
	inline Matrix4x4 operator*(const Matrix4x4& pMatrix) const { return MultiplyByMatrix(pMatrix); }
};

// Quaternion operations
inline Quaternion QuaternionFromAxisAngle(Vector3D& vecAxisOfRotation, f32 flAngle)
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

	f32 flSinX = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.x) / 2);
	f32 flCosX = M_COS_F(M_DEG2RAD_F(vecEulerAngles.x) / 2);
	f32 flSinY = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.y) / 2);
	f32 flCosY = M_COS_F(M_DEG2RAD_F(vecEulerAngles.y) / 2);
	f32 flSinZ = M_SIN_F(M_DEG2RAD_F(vecEulerAngles.z) / 2);
	f32 flCosZ = M_COS_F(M_DEG2RAD_F(vecEulerAngles.z) / 2);

	outQuat.x = flCosX * flSinY * flCosZ + flSinX * flCosY * flSinZ;
	outQuat.y = flSinX * flCosY * flCosZ - flCosX * flSinX * flSinZ;
	outQuat.z = flCosX * flCosY * flSinZ - flSinX * flSinY * flCosZ;
	outQuat.w = flCosX * flCosY * flCosZ + flSinX * flCosX * flSinZ;

	return outQuat;
}

DLL_EXPORT bool RayIntersectsPlane(struct Ray_t* pRay, struct Plane* pPlane);