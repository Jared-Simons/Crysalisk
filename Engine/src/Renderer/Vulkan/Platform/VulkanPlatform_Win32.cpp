#include "EngineGlobals.h"
#ifdef PLAT_WINDOWS
#include "VulkanPlatform.h"
#include "../VulkanBackend.h"
#include "Core/Assertions.h"

const char* Platform_GetVulkanExtensions()
{
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}
#endif

