#include "EngineGlobals.h"
#ifdef PLAT_WINDOWS
#include "VulkanPlatform.h"
#include "../VulkanBackend.h"
#include "Core/Assertions.h"

const char* Platform_GetVulkanExtensions()
{
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

bool Platform_CreateVulkanSurface(CVulkanRenderer* renderingContext)
{
	VkWin32SurfaceCreateInfoKHR createInfo = Platform_GetVulkanSurface();

	// TODO: Custom allocator
	if (vkCreateWin32SurfaceKHR(renderingContext->GetVulkanInstance(), &createInfo, nullptr, &renderingContext->GetVulkanSurface()))
	{
		AssertMsg("Failed to create Vulkan Window Surface!");
		return false;
	}

	LOG_DEBUG("Vulkan Surface Created.");
	return true;
}

#endif

