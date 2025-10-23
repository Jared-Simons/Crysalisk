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
