#include <Defines.h>

#ifdef PLAT_WINDOWS
#include "../VulkanRenderer.h"

#include <Core/Logging.h>

#define WIN32_LEAN_AND_MEAN // We need as little in here as possible!
#include <Windows.h>	// This is needed for GetModuleHandle(), 
						// only other alternative is to pass it in w/ the renderer config like the hwnd.

namespace Platform {

	const char* GetVulkanPlatformExtension()
	{
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}

	// TODO: Combine this with the below method.
	bool CreateVulkanWindowSurface(CVulkanRenderer& pRenderingContext, Platform::IPlatformWindow& pWindow)
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		surfaceCreateInfo.hinstance = GetModuleHandleA(0);
		surfaceCreateInfo.hwnd = static_cast<HWND>(pWindow.GetInternalHandle());

		VK_CHECK(vkCreateWin32SurfaceKHR(pRenderingContext.m_Instance, &surfaceCreateInfo, pRenderingContext.m_Allocator, &((VulkanWindowState_t*)pWindow.m_pRendererState)->m_WindowSurface));

		LOG_DEBUG("Vulkan surface created");
		return true;
	}

	bool CreateVulkanSurface(CVulkanRenderer& pRenderingContext, Platform::IPlatformWindow& pWindow)
	{
		VulkanWindowState_t* pWindowState = (VulkanWindowState_t*)pWindow.m_pRendererState;

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		surfaceCreateInfo.hinstance = GetModuleHandleA(0);
		surfaceCreateInfo.hwnd = static_cast<HWND>(pWindow.GetInternalHandle());

		VK_CHECK(vkCreateWin32SurfaceKHR(pRenderingContext.m_Instance, &surfaceCreateInfo, pRenderingContext.m_Allocator, &pWindowState->m_WindowSurface));
		
		LOG_DEBUG("Vulkan surface created");
		return true;
	}

}

#endif