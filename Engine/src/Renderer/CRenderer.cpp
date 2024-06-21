//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Systems/IRenderer.h"

#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Vulkan/VulkanBackend.h"

#include <Memory/LinearAllocator.h>

class CRenderer : public IRenderer
{
public:
	CRenderer() {}

	void* operator new(size_t size, LinearAllocator* pAllocator)
	{
		void* block = pAllocator->allocate(size);
		if (block)
		{
			return block;
		}
		else
		{
			LOG_FATAL("Systems Allocator failed to allocate memory for the renderer system!");
			return nullptr;
		}
	}

	virtual bool	Initialize() override;
	virtual void	Shutdown() override;
	virtual bool	Update() override;

private:
	IRendererBackend* m_RendererBackend{ nullptr };
};

static CRenderer* s_RendererFrontend{ nullptr };
IRenderer* gpRenderer = s_RendererFrontend;

//-----------------------------------------------------------------------------
// Factory method to initialize a renderer.
//-----------------------------------------------------------------------------
bool IRenderer::Renderer_Create(IRenderer* statePtr, LinearAllocator* pAllocator)
{
	if (statePtr)
	{
		LOG_ERROR("Renderer_Create() -> An active Renderer already exists at this location. Aborting!");
		return false;
	}

	// Setup the engine's renderer
	if (statePtr == gpRenderer)
	{
		s_RendererFrontend = new (pAllocator) CRenderer();

		if (!s_RendererFrontend->Initialize())
		{
			return false;
		}
		gpRenderer = s_RendererFrontend;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Factory method to shutdown a renderer.
//-----------------------------------------------------------------------------
void IRenderer::Renderer_Shutdown(IRenderer* statePtr, bool bOwnsMemory)
{
	if (statePtr)
	{
		statePtr->Shutdown();

		if (bOwnsMemory)
		{
			delete statePtr;
			statePtr = nullptr;
		}
	}
}


bool CRenderer::Initialize()
{
	// TODO: This currently only supports Vulkan...
	m_RendererBackend = GetVulkanBackend();

	if (m_RendererBackend != nullptr)
	{
		if (!m_RendererBackend->RendererBackend_Initialize())
		{
			LOG_FATAL("Failed to initialize rendering backend! Application cannot continue.");
			return false;
		}
	}
	else
	{
		LOG_FATAL("No rendering backend was provided, application cannot continue.");
		return false;
	}

	LOG_INFO("Renderer Initialized!");
	return true;
}

void CRenderer::Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
	m_RendererBackend->RendererBackend_Shutdown();
	delete m_RendererBackend;
	m_RendererBackend = nullptr;
}

bool CRenderer::Update()
{
	if (!m_RendererBackend->RendererBackend_Update())
	{
		return false;
	}

	return true;
}
