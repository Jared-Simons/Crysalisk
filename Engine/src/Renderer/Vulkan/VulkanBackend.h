//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"

#include "Core/Logging.h"
#include <CVector.h>

#include <vulkan/vulkan.h>

// Forward declarations
class CVulkanBackend;

class CVulkanBackend : public IRendererBackend
{
public:
	bool	RendererBackend_Initialize() override;
	void	RendererBackend_Shutdown() override;
	bool	RendererBackend_Update() override;

private:
	bool	CreateVulkanInstance();

private:
	// Instance
	VkInstance		m_Instance;

private:
	VkInstance& GetVulkanInstance() { return m_Instance; }
	const CVector<const char*>& GetValidationLayers() { return m_ValidationLayers; }

	// Only enabling Valdiation Layers on debug builds for now
#if _DEBUG
	bool	EnableValidationLayers() const { return true; }
#else
	bool	EnableValidationLayers() const { return false; }
#endif

	bool	RequestValidationLayerSupport();

	CVector<const char*> m_ValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
};

static CVulkanBackend VulkanRenderingContext;

IRendererBackend* GetVulkanBackend();

