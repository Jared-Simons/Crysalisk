//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: Adds Mathematical Vector Utilities.
//=============================================================================
#pragma once

#include <Defines.h>

#include "Math/MathLib.h"

// Vector elements must be within this range to be considered 'equal'.
const float kVectorElementEpsilon = 1.192092896e-07f;

class Vector2D
{
public:
	float x;
	float y;

public:
	Vector2D()
		: x{ 0 }
		, y{ 0 } {}

	Vector2D(float x_, float y_)
		: x{ x_ }
		, y{ y_ } {}

	Vector2D operator+(const float scalar)
	{
		Vector2D outVector{ x, y };
		outVector.x = x + scalar;
		outVector.y = y + scalar;
		return outVector;
	}

	Vector2D operator-(const float scalar)
	{
		Vector2D outVector{ x, y };
		outVector.x = x - scalar;
		outVector.y = y - scalar;
		return outVector;
	}

	bool operator==(const Vector2D& pOther)
	{
		return ((x - pOther.x) < kVectorElementEpsilon &&
				(y - pOther.y) < kVectorElementEpsilon);
	}

	Vector2D operator*(const float scalar)
	{
		Vector2D outVector{ x, y };
		outVector.x = x * scalar;
		outVector.y = y * scalar;
		return outVector;
	}

	void operator*=(const float scalar)
	{
		x = x * scalar;
		y = y * scalar;
	}

	Vector2D operator/(const float scalar)
	{
		Vector2D outVector{ x, y };
		outVector.x = x / scalar;
		outVector.y = y / scalar;
		return outVector;
	}

	void operator/=(const float scalar)
	{
		x = x / scalar;
		y = y / scalar;
	}

	Vector2D operator+(Vector2D& otherVector)
	{
		Vector2D outVector{};
		outVector.x = x + otherVector.x;
		outVector.y = y + otherVector.y;
		return outVector;
	}

	Vector2D operator-(Vector2D& otherVector)
	{
		Vector2D outVector{};
		outVector.x = x - otherVector.x;
		outVector.y = y - otherVector.y;
		return outVector;
	}

	Vector2D operator*(Vector2D& otherVector)
	{
		Vector2D outVector{};
		outVector.x = x * otherVector.x;
		outVector.y = y * otherVector.y;
		return outVector;
	}

	Vector2D operator/(Vector2D& otherVector)
	{
		Vector2D outVector{};
		outVector.x = x / otherVector.x;
		outVector.y = y / otherVector.y;
		return outVector;
	}

	Vector2D operator!()
	{
		return Negate();
	}

	Vector2D Negate() const
	{
		Vector2D outVector{};
		outVector.x = -x;
		outVector.y = -y;
		return outVector;
	}

	void NegateInPlace()
	{
		x = -x;
		y = -y;
	}

	float Length() const
	{
		return M_SQRT_F((x * x) + (y * y));
	}

	Vector2D Normalize() const
	{
		Vector2D outVector{};
		float length = Length();

		outVector.x = x / length;
		outVector.y = y / length;

		return outVector;
	}

	void NormalizeInPlace()
	{
		float length = Length();

		x = x / length;
		y = y / length;
	}

	static float DotProduct(const Vector2D& vecA, const Vector2D& vecB)
	{
		return (vecA.x * vecB.x) + (vecA.y * vecB.y);
	}
};


class Vector3D
{
public:
	float x;
	float y;
	float z;

	Vector3D()
		: x{ 0 }
		, y{ 0 }
		, z{ 0 } {}

	Vector3D(float x_, float y_, float z_)
		: x{ x_ }
		, y{ y_ }
		, z{ z_ } {}

	bool operator==(const Vector3D& pOther)
	{
		return (M_ABS(x - pOther.x) < kVectorElementEpsilon &&
				M_ABS(y - pOther.y) < kVectorElementEpsilon &&
				M_ABS(z - pOther.z) < kVectorElementEpsilon);
	}

	Vector3D operator+(const float scalar)
	{
		x = x + scalar;
		y = y + scalar;
		z = z + scalar;

		return *this;
	}

	Vector3D operator-(const float scalar)
	{
		x = x - scalar;
		y = y - scalar;
		z = z - scalar;

		return *this;
	}

	Vector3D operator*(const float scalar)
	{
		Vector3D outVector;
		outVector.x = x * scalar;
		outVector.y = y * scalar;
		outVector.z = z * scalar;

		return outVector;
	}

	Vector3D operator/(const float scalar)
	{
		Vector3D outVector;
		outVector.x = x / scalar;
		outVector.y = y / scalar;
		outVector.z = z / scalar;

		return outVector;
	}

	Vector3D operator+(Vector3D& otherVector)
	{
		Vector3D outVector{};
		outVector.x = x + otherVector.x;
		outVector.y = y + otherVector.y;
		outVector.z = z + otherVector.z;
		return outVector;
	}

	void operator+=(Vector3D& otherVector)
	{
		x += otherVector.x;
		y += otherVector.y;
		z += otherVector.z;
	}

	void operator-=(Vector3D& otherVector)
	{
		x -= otherVector.x;
		y -= otherVector.y;
		z -= otherVector.z;
	}

	Vector3D operator-(Vector3D& otherVector)
	{
		Vector3D outVector{};
		outVector.x = x - otherVector.x;
		outVector.y = y - otherVector.y;
		outVector.z = z - otherVector.z;
		return outVector;
	}

	Vector3D operator*(Vector3D& otherVector)
	{
		Vector3D outVector{};
		outVector.x = x * otherVector.x;
		outVector.y = y * otherVector.y;
		outVector.z = z * otherVector.z;
		return outVector;
	}

	Vector3D operator/(Vector3D& otherVector)
	{
		Vector3D outVector{};
		outVector.x = x / otherVector.x;
		outVector.y = y / otherVector.y;
		outVector.z = z / otherVector.z;
		return outVector;
	}

	Vector3D operator=(const Vector3D& otherVector)
	{
		Vector3D outVector{};
		x = otherVector.x;
		y = otherVector.y;
		z = otherVector.z;
		return outVector;
	}

	Vector3D Negate() const
	{
		Vector3D outVector{};
		outVector.x = -x;
		outVector.y = -y;
		outVector.z = -z;
		return outVector;
	}

	void NegateInPlace()
	{
		x = -x;
		y = -y;
		z = -z;
	}

	float Length() const
	{
		return sqrt((x * x) + (y * y) + (z * z));
	}

	Vector3D Normalize() const
	{
		Vector3D outVector{x, y, z};

		float flDot = Vector3D::DotProduct(Vector3D(x, y, z), Vector3D(x, y, z));
		float invSqrt = 1.0f / M_SQRT_F(flDot);

		return outVector * invSqrt;
	}

	void NormalizeInPlace()
	{
		Vector3D outVector{ x, y, z };

		float flDot = Vector3D::DotProduct(Vector3D(x, y, z), Vector3D(x, y, z));
		float invSqrt = 1.0f / M_SQRT_F(flDot);

		*this * invSqrt;
	}

	static float DotProduct(Vector3D& vecA, Vector3D& vecB)
	{
		Vector3D tempVec(vecA * vecB);
		return tempVec.x + tempVec.y + tempVec.z;
	}

	static Vector3D CrossProduct(const Vector3D& vecA, const Vector3D& vecB)
	{
		Vector3D outVector{};
		outVector.x = vecA.y * vecB.z - vecA.z * vecB.y;
		outVector.y = vecA.z * vecB.x - vecA.x * vecB.z;
		outVector.z = vecA.x * vecB.y - vecA.y * vecB.x;
		return outVector;
	}

	inline Vector2D AsVector2D()
	{
		return Vector2D(x, y);
	}
};


class Vector4D
{
public:
	float x;
	float y;
	float z;
	float w;
	Vector4D()
		: x{ 0 }
		, y{ 0 }
		, z{ 0 } 
		, w{ 0 } {}

	Vector4D(float x_, float y_, float z_, float w_)
		: x{ x_ }
		, y{ y_ }
		, z{ z_ }
		, w{ w_ } {}

	Vector4D(Vector3D& pOtherVec)
		: x{ pOtherVec.x }
		, y{ pOtherVec.y }
		, z{ pOtherVec.z }
		, w{ 0 } { }

	void operator+(const float scalar)
	{
		x = x + scalar;
		y = y + scalar;
		z = z + scalar;
		w = w + scalar;
	}

	void operator-(const float scalar)
	{
		x = x - scalar;
		y = y - scalar;
		z = z - scalar;
		w = w - scalar;
	}

	void operator*(const float scalar)
	{
		x = x * scalar;
		y = y * scalar;
		z = z * scalar;
		w = w * scalar;
	}

	void operator/(const float scalar)
	{
		x = x / scalar;
		y = y / scalar;
		z = z / scalar;
		w = w / scalar;
	}

	Vector4D operator+( Vector4D& otherVector)
	{
		Vector4D outVector{};
		outVector.x = x + otherVector.x;
		outVector.y = y + otherVector.y;
		outVector.z = z + otherVector.z;
		outVector.w = w + otherVector.w;
		return outVector;
	}

	Vector4D operator-(Vector4D& otherVector)
	{
		Vector4D outVector{};
		outVector.x = x - otherVector.x;
		outVector.y = y - otherVector.y;
		outVector.z = z - otherVector.z;
		outVector.w = w - otherVector.w;
		return outVector;
	}

	Vector4D operator*(Vector4D& otherVector)
	{
		Vector4D outVector{};
		outVector.x = x * otherVector.x;
		outVector.y = y * otherVector.y;
		outVector.z = z * otherVector.z;
		outVector.w = w * otherVector.w;
		return outVector;
	}

	Vector4D operator/(Vector4D& otherVector)
	{
		Vector4D outVector{};
		outVector.x = x / otherVector.x;
		outVector.y = y / otherVector.y;
		outVector.z = z / otherVector.z;
		outVector.w = w / otherVector.w;
		return outVector;
	}

	Vector4D Negate() const
	{
		Vector4D outVector{};
		outVector.x = -x;
		outVector.y = -y;
		outVector.z = -z;
		outVector.w = -w;
		return outVector;
	}

	void NegateInPlace()
	{
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}

	float Length() const
	{
		return sqrt((x * x) + (y * y) + (z * z) + (w * w));
	}

	Vector4D Normalize() const
	{
		Vector4D outVector{};
		float length = Length();

		outVector.x = x / length;
		outVector.y = y / length;
		outVector.z = z / length;
		outVector.w = w / length;

		return outVector;
	}

	void NormalizeInPlace()
	{
		float length = Length();

		x = x / length;
		y = y / length;
		z = z / length;
		w = w / length;
	}

	static float DotProduct(const Vector4D& vecA, const Vector4D& vecB)
	{
		return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z) + (vecA.w * vecB.w);
	}

	static Vector4D CrossProduct(const Vector4D& vecA, const Vector4D& vecB)
	{
		Vector4D outVector{};
		outVector.x = vecA.y * vecB.z - vecA.z * vecB.y;
		outVector.y = vecA.z * vecB.x - vecA.x * vecB.z;
		outVector.z = vecA.x * vecB.y - vecA.y * vecB.x;
		return outVector;
	}
};

