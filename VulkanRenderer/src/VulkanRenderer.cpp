//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#include <stdio.h>
#include "VulkanRenderer.h"

#include <Defines.h>

#include <Core/Memory.h>

DLL_EXPORT int ConnectInterface(void* pInterfaceState)
{
	if (pInterfaceState)
	{
		//pInterfaceState = gpMemory->Allocate(sizeof(CVulkanRenderer), eMemAllocCategories::MEM_CATEGORY_ENGINE); // TODO: RENDERER TAG
	}

	printf("VulkanRenderer->ConnectInterface\n");
	return 1;
}