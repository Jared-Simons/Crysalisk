//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Platform-specific Utility functions for Vulkan
//=============================================================================
#pragma once

#include <vulkan/vulkan.h>

// Forward Declarations
class CVulkanRenderer;

namespace Platform {
	const char* GetVulkanExtensions();

	bool CreateVulkanSurface(CVulkanRenderer* renderingContext);

#ifdef PLAT_WINDOWS
	VkWin32SurfaceCreateInfoKHR GetVulkanSurface();
#endif
}
