//=============================================================================
// Author: Jared Simons
// Date Created: 7/31/2025
// 
// Purpose: Vulkan Win32 Platform Utilities.
//=============================================================================
#include "VulkanPlatform.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace VulkanPlatform
{
	VkSurfaceKHR CreateWindowSurface(VkInstance& pInstance, void* pHWND)
	{
		VkSurfaceKHR newSurface{};
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		surfaceCreateInfo.hwnd = (HWND)pHWND;
		surfaceCreateInfo.hinstance = GetModuleHandleA(0);

		if (!pHWND)
		{
			// TODO: Logging
			return VK_NULL_HANDLE;
		}

		VkResult surfaceResult = vkCreateWin32SurfaceKHR(pInstance, &surfaceCreateInfo, nullptr, &newSurface);
		if (surfaceResult != VK_SUCCESS)
		{
			// TODO: Logging
			return VK_NULL_HANDLE;
		}

		return newSurface;
	}

	const char* GetPlatformSurfaceExtension()
	{
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}
}
