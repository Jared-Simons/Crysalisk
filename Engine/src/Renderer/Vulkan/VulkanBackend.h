//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"

#include "Core/Logging.h"

// Forward declarations
class CVulkanBackend;

class CVulkanBackend : public IRendererBackend
{
public:
	bool	RendererBackend_Initialize() override;
	void	RendererBackend_Shutdown() override;
	bool	RendererBackend_Update() override;
};


bool CVulkanBackend::RendererBackend_Initialize()
{
	LOG_DEBUG("Vulkan Rendering Backend Initialized");
	return true;
}

void CVulkanBackend::RendererBackend_Shutdown()
{
	LOG_DEBUG("Vulkan Rendering Backend Shutting Down");
}

bool CVulkanBackend::RendererBackend_Update()
{
	return true;
}

static CVulkanBackend VulkanRenderingContext;

IRendererBackend* GetVulkanBackend()
{
	return &VulkanRenderingContext;
}

