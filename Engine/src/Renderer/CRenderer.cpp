//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Systems/IRenderer.h"

#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Resources/ResourceTypes.h"
#include "Vulkan/VulkanBackend.h"

#include "Systems/TextureSystem.h"

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

	// NOTE: This does nothing and just exists to clear a compiler warning, 
	// the memory was allocated and will be freed when the linear allocator gets cleared.
	// If the creation of this system fails, the engine will shutdown regardless.
	void	operator delete(void* pMemory, LinearAllocator* pAllocator) {}

	virtual bool	Initialize() override;
	virtual void	Shutdown() override;
	virtual bool	Update(struct RenderPacket_t* pRenderData) override;

	bool	CreateTextureResources(Texture& pTexture, unsigned char* pPixelData) override;
	void	DestroyTextureResources(Texture& pTexture) override;

	bool	CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<unsigned short>& indices, const char* szTextureName, const char* szGeometryName) override;
	void	DestroyGeometryResources(StaticMesh& pMesh) override;

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

bool CRenderer::Update(RenderPacket_t* pRenderData)
{
	if (!m_RendererBackend->RendererBackend_Update(pRenderData))
	{
		return false;
	}

	return true;
}

bool CRenderer::CreateTextureResources(Texture& pTexture, unsigned char* pPixelData)
{
	if (!pPixelData)
	{
		LOG_ERROR("CreateTextureResources requires a valid pointer to pixel data. Texture creation failed.");
		return false;
	}

	return m_RendererBackend->CreateTextureResources(pTexture, pPixelData);
}

void CRenderer::DestroyTextureResources(Texture& pTexture)
{
	m_RendererBackend->DestroyTextureResources(pTexture);
}

bool CRenderer::CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<unsigned short>& indices, const char* szTextureName, const char* szGeometryName)
{
	// Acquire necessary textures
	pMesh.m_pTexture = gpTextureSystem->AcquireTexture(szTextureName);
	pMesh.m_szGeometryName = szGeometryName;

	return m_RendererBackend->CreateGeometryResources(pMesh, vertices, indices);
}

void CRenderer::DestroyGeometryResources(StaticMesh& pMesh)
{
	m_RendererBackend->DestroyGeometryResources(pMesh);
}
