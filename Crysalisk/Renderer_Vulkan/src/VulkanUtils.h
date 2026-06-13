//=============================================================================
// Author: Jared Simons
// Date Created: 7/9/2025
// 
// Purpose: Vulkan Renderer Utilities.
//=============================================================================
#pragma once

// Enable this to mark every Vulkan resource object with tags for viewing in RenderDoc.
// This is separate from _DEBUG so that we can debug Vulkan frames with the speed of Distribution builds.
#define VULKAN_RENDERER_RESOURCE_DEBUGGING_ENABLED

#ifdef VULKAN_RENDERER_RESOURCE_DEBUGGING_ENABLED
	#define VULKAN_RENDERER_NAME_OBJECT(pRenderingContext, pszObjectName, pObjectType, pObjectHandle)	\
		VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };	\
		nameInfo.objectType = pObjectType;																\
		nameInfo.objectHandle = (u64)pObjectHandle;												\
		nameInfo.pObjectName = pszObjectName;															\
		if (pRenderingContext.m_pfnSetDebugObjectName)													\
			pRenderingContext.m_pfnSetDebugObjectName(pRenderingContext.m_Device.m_Device, &nameInfo);	
#else
	#define VULKAN_RENDERER_NAME_OBJECT(pRenderingContext, pszObjectName, objectType, objectHandle)	
		// Do nothing here!
#endif
