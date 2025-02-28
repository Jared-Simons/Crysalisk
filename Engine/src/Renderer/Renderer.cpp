//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Renderer/Renderer.h"

#include "Core/Event.h"
#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Platform/Platform.h"
#include "Resources/GeometryGenerator.h"
#include "Resources/ResourceTypes.h"
#include "Systems/TextureSystem.h"

// Renderer frontend context.
static Renderer s_Renderer;
Renderer* g_pRenderer = &s_Renderer;

// Event handlers.
static bool RendererEvent_WindowResized(EventData_t data);

// A callback to be used to get an opaque pointer to the renderer-backend's state.
typedef void* (*PFN_GetRendererBackendState)();

//-------------------------------------------------------------------------
// Purpose: Initialize the Renderer frontend and backend.
// Input: pConfig - The configuration for both the frontend and backend.
// Output: bool - Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool Renderer::Initialize(RendererConfig_t& pConfig)
{
	// Load up the renderer backend module, and connect our interfaces.
	Platform::ISysModule* pVulkanRendererDLL = pConfig.pRendererBackendDLL;

	PFN_GetRendererBackendState getRendererBackend = (PFN_GetRendererBackendState)pVulkanRendererDLL->GetFuncAddr("GetRendererBackendState");
	if (!getRendererBackend)
	{
		LOG_ERROR("Renderer::Initialize failed to get a pointer to the renderer backend. Initialization failed.");
		return false;
	}

	// TODO: This is still not great. Would probably be better to use the IRendererBackend interface or something to get at the state.
	m_RendererBackend = (IRendererBackend*)getRendererBackend();

	if (m_RendererBackend != nullptr)
	{
		if (!m_RendererBackend->RendererBackend_Initialize(pConfig))
		{
			return false;
		}
	}
	else
	{
		LOG_FATAL("No rendering backend was provided, application cannot continue.");
		return false;
	}

	g_pEngineEvents_Internal->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);

	LOG_INFO("Renderer Initialized!");
	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the renderer system.
//-------------------------------------------------------------------------
void Renderer::Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
	m_RendererBackend->RendererBackend_Shutdown();

	m_RendererBackend = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Draws a frame with the renderer.
// Input: pRenderData - Frame-specific data to be used in rendering ops.
// Output: bool - True on successful frame; if there were any rendering errors, false.
//-------------------------------------------------------------------------
bool Renderer::Update(RenderPacket_t& pRenderData)
{
	// If we are currently minimized, do not do anything!
	if (m_bMinimized)
	{
		return true;
	}

	if (!m_RendererBackend->StartFrame(pRenderData))
	{
		return false;
	}

	// Render our temp geometry.
	m_RendererBackend->DrawGeometry(m_Cube1);
	m_RendererBackend->DrawGeometry(m_Cube2);
	m_RendererBackend->DrawGeometry(m_Cube3);

	m_RendererBackend->EndFrame(pRenderData);

	return true;
}

bool Renderer::AcquireTextureResources(Texture& pTexture, size_t nPixelDataSize, byte* pPixelData)
{
	// No work to be done here, pass it right through to the backend.
	return m_RendererBackend->AcquireTextureResources(pTexture, nPixelDataSize, pPixelData);
}

void Renderer::ReleaseTextureResources(Texture& pTexture)
{
	m_RendererBackend->ReleaseTextureResources(pTexture);
}

bool Renderer::CreateTempGeometry()
{
	GeometryConfig_t cubeConfig = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, 0 }, 32, 32, 32);
	GeometryConfig_t cubeConfig2 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, -50 }, 32, 32, 32);
	GeometryConfig_t cubeConfig3 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, 50 }, 32, 32, 32);

	if (!m_RendererBackend->AcquireGeometryResources(cubeConfig, m_Cube1))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}

	if (!m_RendererBackend->AcquireGeometryResources(cubeConfig2, m_Cube2))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}

	if (!m_RendererBackend->AcquireGeometryResources(cubeConfig3, m_Cube3))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}

	// Acquire texture
	m_Cube2.pTexture = g_pTextureSystem->LoadTexture("Assets/Textures/container.jpg");
	m_Cube3.pTexture = g_pTextureSystem->LoadTexture("Assets/Textures/container2.png");

	return true;
}

static bool RendererEvent_WindowResized(EventData_t data)
{
	// Handle minimized contexts.
	if (data.u32[0] == 0 || data.u32[1] == 0)
	{
		s_Renderer.SetMinimized(true);
	}
	else
	{
		s_Renderer.SetMinimized(false);
	}

	// Let other listeners handle this event.
	return false;
}