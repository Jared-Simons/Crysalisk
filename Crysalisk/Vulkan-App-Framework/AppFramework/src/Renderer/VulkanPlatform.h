//=============================================================================
// Author: Jared Simons
// Date Created: 7/31/2025
// 
// Purpose: Vulkan Platform Utilities.
//=============================================================================
#pragma once

#include <vulkan/vulkan.h>

namespace VulkanPlatform
{
	VkSurfaceKHR	CreateWindowSurface(VkInstance& pInstance, void* pHWND);
	const char*		GetPlatformSurfaceExtension();
}