//=============================================================================
// Author: Jared Simons
// Date Created: 10/7/2024
// 
// Purpose: Definitions for core types to be used throughout the codebase.
// This file should be included pretty much everywhere.
//=============================================================================
#pragma once

#include <cstdint>

//-----------------------------------------------
// Primitive Typedefs
//-----------------------------------------------

// Unsigned integer types
typedef uint8_t			u8;
typedef uint16_t		u16;
typedef uint32_t		u32;
typedef uint64_t		u64;

// Signed integer types
typedef int8_t			i8;
typedef int16_t			i16;
typedef int32_t			i32;
typedef int64_t			i64;

// Floating point types
typedef float			f32;
typedef double			f64;

typedef unsigned char byte;

// Type assertions
// These exist to ensure that our primitive typedefs will be the correct size at compile time.
// If this causes the compile to fail, then the types will need to be redefined for the platform.
static_assert(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
static_assert(sizeof(u16) == 2, "Expected u8 to be 2 bytes.");
static_assert(sizeof(u32) == 4, "Expected u8 to be 4 bytes.");
static_assert(sizeof(u64) == 8, "Expected u8 to be 8 bytes.");

static_assert(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
static_assert(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
static_assert(sizeof(i32) == 4, "Expected int to be 4 bytes.");
static_assert(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

static_assert(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
static_assert(sizeof(f64) == 8, "Expected f32 to be 8 bytes.");

// Function imports/exports
#if defined CORE_DLL
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif

#if defined ENGINE_DLL
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

#if defined GAME_DLL
#define GAME_DLL __declspec(dllexport)
#else
#define GAME_DLL  __declspec(dllimport)
#endif

#if defined VULKAN_DLL
#define VULKAN_API  __declspec(dllexport)
#else
#define VULKAN_API  __declspec(dllimport)
#endif

#define DLL_EXPORT extern "C" __declspec(dllexport)

// Platform detection

// Windows
#if defined _WIN32
#define PLAT_WINDOWS 1
#else
#define PLAT_WINDOWS 0
#endif

// Memory Macros

#define KIBIBYTES(x) (x * 1024)
#define MEBIBYTES(x) (x * (1024 * 1024))
#define GIBIBYTES(x) (x * (1024 * 1024 * 1024))

// Invalid IDs
const unsigned int kInvalidID_U8 = 255;
const unsigned short kInvalidID_U16 = 65535;
const unsigned int kInvalidID_U32 = 4294967294;
const unsigned long long kInvalidID_U64 = 18446744073709551614;

// Limits
const float kFLT_MAX = 3.402823466e+38f;
const float kFLT_MIN = 1.175494351e-38f;

// Utility Functions

static inline bool IsNumberAPowerOfTwo(size_t x) { return (x & (x - 1)) == 0; }

static inline void IntClamp(i32* val, i32* min, i32* max)
{
	if (val < min)
	{
		val = min;
	}
	else if (val > max)
	{
		val = max;
	}
}

static inline int IntMin(int a, int b)
{
	return (a < b) ? a : b;
}

static inline int IntMax(int a, int b)
{
	return (a > b) ? a : b;
}

static inline float FloatMin(float a, float b)
{
	return (a < b) ? a : b;
}

static inline float FloatMax(float a, float b)
{
	return (a > b) ? a : b;
}
