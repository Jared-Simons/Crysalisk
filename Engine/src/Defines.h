//=============================================================================
// Author: Jared Simons
// Date Created: 10/7/2024
// 
// Purpose: Definitions for core types to be used throughout the codebase.
// This file should be included pretty much everywhere.
//=============================================================================
#pragma once

//-----------------------------------------------
// Primitive Typedefs
//-----------------------------------------------

// Unsigned integer types
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

// Signed integer types
typedef signed char			i8;
typedef signed short		i16;
typedef signed int			i32;
typedef signed long long	i64;

// Floating point types
typedef float				f32;
typedef double				f64;

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
#if defined ENGINE_DLL
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API  __declspec(dllimport)
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
#define INVALID_ID_U8	255

// Utility Functions

static inline bool IsNumberAPowerOfTwo(size_t x) { return (x & (x - 1)) == 0; }

static void intClamp(i32* val, i32* min, i32* max)
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