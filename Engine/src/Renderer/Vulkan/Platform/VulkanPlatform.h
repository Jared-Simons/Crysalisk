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

const char* Platform_GetVulkanExtensions();

void Platform_CreateVulkanSurface(CVulkanRenderer* renderingContext);

#ifdef PLAT_WINDOWS
VkWin32SurfaceCreateInfoKHR Platform_GetVulkanSurface();
#endif

