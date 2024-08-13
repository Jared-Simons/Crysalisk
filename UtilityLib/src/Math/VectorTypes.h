//=============================================================================
// Author: Jared Simons
// Date Created: 5/15/2024
// 
// Purpose: Adds Mathematical Vector Utilities.
//=============================================================================
#pragma once

// TODO: Replace this with our own header at some point
#include <cmath>

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

	void operator+(const float scalar)
	{
		x = x + scalar;
		y = y + scalar;
	}

	void operator-(const float scalar)
	{
		x = x - scalar;
		y = y - scalar;
	}

	void operator*(const float scalar)
	{
		x = x * scalar;
		y = y * scalar;
	}

	void operator/(const float scalar)
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
		return sqrt((x * x) + (y * y));
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
		x = x * scalar;
		y = y * scalar;
		z = z * scalar;

		return *this;
	}

	Vector3D operator/(const float scalar)
	{
		x = x / scalar;
		y = y / scalar;
		z = z / scalar;

		return *this;
	}

	Vector3D operator+(Vector3D& otherVector)
	{
		Vector3D outVector{};
		outVector.x = x + otherVector.x;
		outVector.y = y + otherVector.y;
		outVector.z = z + otherVector.z;
		return outVector;
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
		Vector3D outVector{};
		float length = Length();

		outVector.x = x / length;
		outVector.y = y / length;
		outVector.z = z / length;

		return outVector;
	}

	void NormalizeInPlace()
	{
		float length = Length();

		x = x / length;
		y = y / length;
		z = z / length;
	}

	static float DotProduct(const Vector3D& vecA, const Vector3D& vecB)
	{
		return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z);
	}

	static Vector3D CrossProduct(const Vector3D& vecA, const Vector3D& vecB)
	{
		Vector3D outVector{};
		outVector.x = vecA.y * vecB.z - vecA.z * vecB.y;
		outVector.y = vecA.z * vecB.x - vecA.x * vecB.z;
		outVector.z = vecA.x * vecB.y - vecA.y * vecB.x;
		return outVector;
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

