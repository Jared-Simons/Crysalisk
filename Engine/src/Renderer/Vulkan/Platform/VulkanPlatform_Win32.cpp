#include "EngineGlobals.h"
#ifdef PLAT_WINDOWS
#include "VulkanPlatform.h"
#include "../VulkanBackend.h"
#include "Core/Logging.h"
#include "Core/Assertions.h"

namespace Platform {

	const char* Platform_GetVulkanExtensions()
	{
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}

	bool Platform_CreateVulkanSurface(CVulkanRenderer* pRenderingContext)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = Platform::Platform_GetVulkanSurface();

		if (!VK_CHECK(vkCreateWin32SurfaceKHR(pRenderingContext->m_Instance, &createInfo, pRenderingContext->m_Allocator, &pRenderingContext->m_Surface)))
		{
			LOG_FATAL("Failed to create Vulkan Window Surface");
			return false;
		}

		LOG_INFO("Vulkan Surface Created");
		return true;
	}
#endif

}