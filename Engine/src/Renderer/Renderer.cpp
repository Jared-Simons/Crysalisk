//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Systems/Renderer.h"

#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Resources/ResourceTypes.h"
#include "Vulkan/VulkanBackend.h"

#include "Systems/TextureSystem.h"

#include <Memory/LinearAllocator.h>

static Renderer* s_Renderer = nullptr;
Renderer* gpRenderer = nullptr;

//-----------------------------------------------------------------------------
// Factory method to initialize a renderer.
//-----------------------------------------------------------------------------
Renderer* Renderer::Renderer_Create(LinearAllocator* pAllocator)
{
	if (s_Renderer)
	{
		LOG_ERROR("Renderer_Create() -> An active Renderer already exists at this location. Aborting!");
		return nullptr;
	}

	Renderer* renderer = new (pAllocator) Renderer();

	s_Renderer = renderer;
	if (!s_Renderer->Initialize())
	{
		return nullptr;
	}

	return s_Renderer;
}

bool Renderer::Initialize()
{
	// TODO: Connect Renderer_Vulkan.dll interface.
	ISysModule* pVulkanRendererDLL = Platform::LoadModule("Renderer_Vulkan.dll");
	pVulkanRendererDLL->LoadFuncAddr("ConnectInterface");
	void* funcAddr = pVulkanRendererDLL->GetLoadedFunctionAddr("ConnectInterface");
	typedef void (*connectInterface)(void* pInterfaceState);
	connectInterface testFunc = reinterpret_cast<connectInterface>(funcAddr);
	testFunc(m_RendererBackend);

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

void Renderer::Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
	m_RendererBackend->RendererBackend_Shutdown();
	delete m_RendererBackend;
	m_RendererBackend = nullptr;
}

bool Renderer::Update(RenderPacket_t* pRenderData)
{
	if (!m_RendererBackend->RendererBackend_Update(pRenderData))
	{
		return false;
	}

	return true;
}

bool Renderer::CreateTextureResources(Texture& pTexture, byte* pPixelData)
{
	if (!pPixelData)
	{
		LOG_ERROR("CreateTextureResources requires a valid pointer to pixel data. Texture creation failed.");
		return false;
	}

	return m_RendererBackend->CreateTextureResources(pTexture, pPixelData);
}

void Renderer::DestroyTextureResources(Texture& pTexture)
{
	m_RendererBackend->DestroyTextureResources(pTexture);
}

bool Renderer::CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices, const char* szTextureName, const char* szGeometryName)
{
	// Acquire necessary textures
	pMesh.m_pTexture = gpTextureSystem->AcquireTexture(szTextureName);
	pMesh.m_szGeometryName = szGeometryName;

	return m_RendererBackend->CreateGeometryResources(pMesh, vertices, indices);
}

void Renderer::DestroyGeometryResources(StaticMesh& pMesh)
{
	m_RendererBackend->DestroyGeometryResources(pMesh);
	gpTextureSystem->UnloadTexture(*pMesh.m_pTexture); // TODO: Release reference instead
}
