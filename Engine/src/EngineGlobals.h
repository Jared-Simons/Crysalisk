#pragma once

// Exports
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

// Silence Certain Warnings
// (ONLY DO THIS IF THERE IS A REASON FOR IT!)


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

static void IntClamp(int* val, int* min, int* max)
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