//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Renderer/Renderer.h"

#include "Core/Event.h"
#include "Core/Input.h"
#include "Core/Logging.h"
#include "IRendererBackend.h"
#include "Platform/Platform.h"
#include "Resources/GeometryGenerator.h"
#include "Resources/ResourceTypes.h"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"

// Renderer frontend context.
static Renderer s_Renderer;
Renderer* g_pRenderer = &s_Renderer;

// Event handlers.
static bool RendererEvent_WindowResized(EventData_t data);
static bool RendererEvent_KeyPressed(EventData_t data);

// A callback to be used to get an opaque pointer to the renderer-backend's state.
typedef void* (*PFN_GetRendererBackendState)();

// TEMP!
ShaderHandle_t testShaderHandle = kInvalidID_U32;
// END TEMP!

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

	// Get an opaque pointer to the renderer backend.
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
	g_pEngineEvents_Internal->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);

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
	DrawGeometry(m_Cube1);
	DrawGeometry(m_Cube2);
	DrawGeometry(m_Cube3);

	m_RendererBackend->EndFrame(pRenderData);

	return true;
}

bool Renderer::AcquireTextureResources(Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	// No work to be done here, pass it right through to the backend.
	return m_RendererBackend->AcquireTextureResources(pTexture, internalIndex, nPixelDataSize, pPixelData);
}

void Renderer::ReleaseTextureResources(Texture& pTexture)
{
	m_RendererBackend->ReleaseTextureResources(pTexture);
}

bool Renderer::AcquireMaterialResources(class Material& pMaterial, struct MaterialInstanceData_t& pMaterialInstanceData)
{
	return m_RendererBackend->AcquireMaterialResources(pMaterial, pMaterialInstanceData);
}

bool Renderer::AcquireShaderResources(Shader& pShader, ShaderConfig_t& pShaderConfig)
{
	return m_RendererBackend->AcquireShaderResources(pShader, pShaderConfig);
}

void Renderer::ReleaseShaderResources(Shader& pShader)
{
	return m_RendererBackend->ReleaseShaderResources(pShader);
}

void Renderer::BindShader(Shader& pShader)
{
	m_RendererBackend->BindShader(pShader);
}

void Renderer::DrawGeometry(GeometryInstance_t& pGeometry)
{
	// Acquire the default material if our own material's resources aren't yet available.
	MaterialReference& pMaterial = (pGeometry.hMaterial.m_iInternalHandle == kInvalidID_U32) ? g_pMaterialSystemInternal->GetDefaultMaterialReference() : pGeometry.hMaterial;

	m_RendererBackend->DrawGeometry(pGeometry, pMaterial);
}

bool Renderer::CreateTempGeometry()
{
	GeometryConfig_t cubeConfig = MeshGenerator::CreateDefaultCubeConfiguration({ 50, 0, 50 }, 20, 20, 20);
	GeometryConfig_t cubeConfig2 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, 50 }, 20, 20, 20);
	GeometryConfig_t cubeConfig3 = MeshGenerator::CreateDefaultCubeConfiguration({ 60, 40, 0 }, 32, 32, 32);

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

	// Acquire materials
	m_Cube1.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial");
	//m_Cube2.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial");
	//m_Cube2.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial2");
	m_Cube3.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial3");

	// TEMP SHADER TEST CODE
	//testShaderHandle = g_pShaderSystem->AcquireShader("CoreShader");
	// END TEMP

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

static bool RendererEvent_KeyPressed(EventData_t data)
{
	if (*data.c == 'T' && s_Renderer.m_Cube2.hMaterial.m_iInternalHandle == kInvalidID_U32)
	{
		LOG_DEBUG("Loading test material.");
		s_Renderer.m_Cube2.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial2");
	}

	// Let other listeners handle this event.
	return false;
}