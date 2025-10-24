//=============================================================================
// Author: Jared Simons
// Date Created: 8/5/2024
// 
// Purpose: Implements core math functionality
//=============================================================================
#pragma once

#include <Defines.h>

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

DLL_EXPORT float M_RAD2DEG_F(float angleInRadians);
DLL_EXPORT double M_RAD2DEG(double angleInRadians);

DLL_EXPORT float M_DEG2RAD_F(float angleInDegrees);
DLL_EXPORT double M_DEG2RAD(double angleInDegrees);

DLL_EXPORT float M_ABS_F(float x);
DLL_EXPORT double M_ABS(double x);

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
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		Vector2D outVector{ *this };
		float flLength = outVector.Length();
		for (int idx = 0; idx < 2; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		float flLength = Length();
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

	// NewMathTODO: This operation should be communicative... is it possible to define it to take the arguments in either order?
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

	// ----------------
	// Vector Normalize
	// ----------------
	inline Vector3D Normalize() const
	{
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		Vector3D outVector{ *this };
		float flLength = outVector.Length();
		for (int idx = 0; idx < 3; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		float flLength = Length();
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
	
	// NewMathTODO: This operation should be communicative... is it possible to define it to take the arguments in either order?
	inline Vector3D operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
	inline void operator*=(const float flScalar) { MultiplyByScalarInPlace(flScalar); }
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
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		Vector4D outVector{ *this };
		float flLength = outVector.Length();
		for (int idx = 0; idx < 4; idx++)
		{
			outVector.flElements[idx] /= flLength;
		}
		return outVector;
	}

	inline void NormalizeInPlace()
	{
		// NewMathTODO: Should this check/assert against division by zero? (Probably...)
		float flLength = Length();
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

// ------------------
// Operator overloads
// ------------------
inline Vector4D operator+(const Vector3D& pOtherVector) const { return Add(pOtherVector); }
inline Vector4D operator-(const Vector3D& pOtherVector) const { return Subtract(pOtherVector); }
inline Vector4D operator-() const { return Negate(); }

// NewMathTODO: This operation should be communicative... is it possible to define it to take the arguments in either order?
inline Vector4D operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
inline void operator*=(const float flScalar) { MultiplyByScalarInPlace(flScalar); }
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
	// NewMathLEFTOFF: Re-evaluate this and determine if this is necessary?
	inline Matrix4x4 Inverse() const
	{
		Matrix4x4 outMatrix{ *this };
		float flCofactors[16]{ 0.0f };

		// First, compute the cofactors.
		flCofactors[0] = (outMatrix.flData[5] * outMatrix.flData[10] * outMatrix.flData[15]) + (outMatrix.flData[6] * outMatrix.flData[11] * outMatrix.flData[13]) + (outMatrix.flData[7] * outMatrix.flData[9] * outMatrix.flData[14]
			- (outMatrix.flData[7] * outMatrix.flData[10] * outMatrix.flData[13]) - (outMatrix.flData[6] * outMatrix.flData[9] * outMatrix.flData[15]) - (outMatrix.flData[5] * outMatrix.flData[11] * outMatrix.flData[14]));
		flCofactors[1] = (outMatrix.flData[4] * outMatrix.flData[10] * outMatrix.flData[15]) + (outMatrix.flData[6] * outMatrix.flData[11] * outMatrix.flData[12]) + (outMatrix.flData[7] * outMatrix.flData[8] * outMatrix.flData[14]
			- (outMatrix.flData[7] * outMatrix.flData[10] * outMatrix.flData[12]) - (outMatrix.flData[6] * outMatrix.flData[8] * outMatrix.flData[15]) - (outMatrix.flData[4] * outMatrix.flData[11] * outMatrix.flData[14]));

		return outMatrix;
	}

	// ------------------
	// Operator overloads
	// ------------------
	inline Matrix4x4 operator*(const float flScalar) const { return MultiplyByScalar(flScalar); }
};
