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
#include "Renderer/Camera.h"
#include "Resources/GeometryGenerator.h"
#include <Resources/Loaders/MeshLoader.h> // TODO: Move this to a mesh manager.
#include "Resources/ResourceTypes.h"
#include "Systems/FontSystem.h"
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
static Camera_t s_MainCamera;

TextObject	s_FontTest{};
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

	// Set up our renderpasses
	m_SceneView.m_bShouldClear = true; // We do want to clear the framebuffer before rendering the scene.
	m_RendererBackend->CreateRendererViewInternal(m_SceneView);

	m_UIView.m_bShouldClear = false;
	m_RendererBackend->CreateRendererViewInternal(m_UIView);

	// Set up our renderbuffers
	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex3D) * 5 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, m_WorldVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 5 * 1024 * 1024, eRenderBufferUsage::INDEX, true, m_WorldIndexBuffer);

	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex2D) * 1 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, m_UIVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 1 * 1024 * 1024, eRenderBufferUsage::INDEX, true, m_UIIndexBuffer);

	g_pEngineEvents_Internal->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);
	g_pEngineEvents_Internal->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);

	s_MainCamera.RecalculateCameraVectors();

	// Start the clock now and we'll take the delta time each frame.
	m_RendererClock.Start();

	m_iCurrentRendererFrame = 0;

	LOG_INFO("Renderer Initialized!");
	return true;
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the renderer system.
//-------------------------------------------------------------------------
void Renderer::Shutdown()
{
	LOG_INFO("Shutting Down Renderer");
	m_RendererBackend->DestroyRenderBufferInternal(m_UIIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_UIVertexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldVertexBuffer);

	m_RendererBackend->DestroyRendererViewInternal(m_UIView);
	m_RendererBackend->DestroyRendererViewInternal(m_SceneView);

	m_RendererBackend->RendererBackend_Shutdown();

	m_RendererBackend = nullptr;
}

extern bool s_bLevelGeometryLoaded;
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
	// Calculate our delta time.
	double currentFrameTime = m_RendererClock.GetElapsedTime();
	pRenderData.DeltaTime = currentFrameTime - m_flLastFrameTime;
	m_flLastFrameTime = currentFrameTime;

	s_MainCamera.UpdateCameraData(pRenderData.DeltaTime);

	if (!m_RendererBackend->StartFrame(pRenderData))
	{
		return false;
	}

	m_RendererBackend->StartRenderpass(m_SceneView);

	// TEMP BADNESS
	g_pShaderSystem->BindShader(0);

	Matrix4x4 projection = Matrix4x4::Perspective(M_DEG2RAD_F(45.0f), 800 / 600.0f, 0.1f, 10000.0f); // TODO: Don't hardcode this.
	// Flip the y-axis.
	projection.data[5] *= -1;
	Matrix4x4 view = s_MainCamera.GetViewMatrix();
	Vector3D myPos = s_MainCamera.vecPosition;

	// TODO: Get these shader locations set in here.
	// TODO: Don't hardcode these things here
	g_pShaderSystem->SetShaderUniform(0, 0, 0, 0, &projection);
	g_pShaderSystem->SetShaderUniform(0, 1, 0, 0, &view);
	g_pShaderSystem->SetShaderUniform(0, 2, 0, 0, &myPos);

	if (s_bLevelGeometryLoaded)
	{
		// TODO: Make this work with an allocator.
		Utl::CVector<GeometryInstance_t*> pOpaqueGeometries(512);
		Utl::CVector<GeometryInstance_t*> pTransparentGeometries(512);

		for (int idx = 0; idx < m_SponzaModel.m_Geometries.Count(); idx++)
		{
			if (!m_SponzaModel.m_Geometries[idx].bIsTransparent)
			{
				// If not transparent, add them to the list!
				pOpaqueGeometries.PushBack(&m_SponzaModel.m_Geometries[idx]);
			}
			else
			{
				// Otherwise, add it to the transparent list to be sorted prior to drawing.
				pTransparentGeometries.PushBack(&m_SponzaModel.m_Geometries[idx]);
			}
		}

		// Render our temp geometry.

		// Bind the appropriate material.
		// TODO: Don't hardcode this material handle here.
		//g_pMaterialSystemInternal->ApplyMaterialInstance(0);
		//DrawGeometry(m_Cube1);
		//DrawGeometry(m_Cube2);
		//DrawGeometry(m_Cube3);

		// Render the opaque geometry first.
		for (int idx = 0; idx < pOpaqueGeometries.Count(); idx++)
		{
			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pOpaqueGeometries[idx]->hMaterial);
			DrawGeometry(*pOpaqueGeometries[idx]);
		}

		// TODO: Sort the transparent geometries from farthest to closest to the camera.

		// Render the sorted transparent geometries.
		for (int idx = 0; idx < pTransparentGeometries.Count(); idx++)
		{
			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pTransparentGeometries[idx]->hMaterial);
			DrawGeometry(*pTransparentGeometries[idx]);
		}
	}

	// TEMP BADNESS
	//g_pShaderSystem->BindShader(0);
	//g_pShaderSystem->BindPerInstanceUniforms(0);
	//g_pMaterialSystemInternal->ApplyMaterialInstance(0);
	/*for (int idx = 0; idx < m_SponzaModel.m_Geometries.Count(); idx++)
	{
		// Bind the appropriate material.
		g_pMaterialSystemInternal->ApplyMaterialInstance(m_SponzaModel.m_Geometries[idx].hMaterial);
		DrawGeometry(m_SponzaModel.m_Geometries[idx]);
	}*/

	m_RendererBackend->EndRenderpass();

	/*if (s_bLevelGeometryLoaded)
	{
		m_RendererBackend->StartRenderpass(m_UIView);

		// Render the test ui text
		m_RendererBackend->RenderTestUI(m_UIVertexBuffer, m_UIIndexBuffer, s_FontTest.m_Geometry);

		m_RendererBackend->EndRenderpass();
	}*/

	m_RendererBackend->EndFrame(pRenderData);

	// Increment the frame count.
	m_iCurrentRendererFrame++;

	return true;
}

bool Renderer::AcquireTextureResources(Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	return m_RendererBackend->AcquireTextureResources(pTexture, internalIndex, nPixelDataSize, pPixelData);
}

void Renderer::ReleaseTextureResources(Texture* pTexture)
{
	m_RendererBackend->ReleaseTextureResources(pTexture);
}

bool Renderer::AcquireTextureResources_OLD(Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	// No work to be done here, pass it right through to the backend.
	return m_RendererBackend->AcquireTextureResources_OLD(pTexture, internalIndex, nPixelDataSize, pPixelData);
}

void Renderer::ReleaseTextureResources_OLD(Texture& pTexture)
{
	m_RendererBackend->ReleaseTextureResources_OLD(pTexture);
}

bool Renderer::AcquireMaterialResources_OLD(class Material& pMaterial, struct MaterialInstanceData_t& pMaterialInstanceData)
{
	return m_RendererBackend->AcquireMaterialResources(pMaterial, pMaterialInstanceData);
}

bool Renderer::CreateShader(Shader* pShader, ShaderConfig_t& pConfig)
{
	return m_RendererBackend->CreateShader(pShader, pConfig); 
}

void Renderer::DestroyShader(Shader* pShader)
{
	m_RendererBackend->DestroyShader(pShader);
}

void Renderer::AcquireShaderInstanceResources(Shader* pShader, unsigned int* pOutInstanceID)
{
	m_RendererBackend->AcquireShaderInstanceResources(pShader, pOutInstanceID);
}

void Renderer::BindShader(Shader* pShader)
{
	m_RendererBackend->BindShader(pShader);
}

void Renderer::BindShaderPerFrameUniforms(Shader* pShader)
{
	m_RendererBackend->BindPerFrameShaderDescriptors(pShader);
}

void Renderer::BindShaderPerInstanceUniforms(Shader* pShader, unsigned int hInstanceID)
{
	m_RendererBackend->BindPerInstanceShaderDescriptors(pShader, hInstanceID);
}

void Renderer::SetShaderUniform(Shader* pShader, ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData)
{
	m_RendererBackend->SetShaderUniform(pShader, pUniform, iUniformArrayIndex, hInstanceID, pData);
}

bool Renderer::AcquireShaderResources_OLD(Shader& pShader, ShaderConfig_t& pShaderConfig)
{
	return m_RendererBackend->AcquireShaderResources_OLD(pShader, pShaderConfig);
}

void Renderer::ReleaseShaderResources(Shader& pShader)
{
	//return m_RendererBackend->ReleaseShaderResources(pShader);
	m_RendererBackend->DestroyShader(&pShader);
}

void Renderer::BindShader_OLD(Shader& pShader)
{
	m_RendererBackend->BindShader_OLD(pShader);
}

void Renderer::AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, pGeometryConfig, pOutInstance);
}

void Renderer::AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(pVertexBuffer, pIndexBuffer, pGeometryConfig, pOutInstance);
}

void Renderer::ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry)
{
	m_RendererBackend->ReleaseGeometryResources(pVertexBuffer, pIndexBuffer, pGeometry);
}

void Renderer::DrawGeometry(GeometryInstance_t& pGeometry)
{
	// Acquire the default material if our own material's resources aren't yet available.
	MaterialReference& pMaterial = (pGeometry.hMaterial_OLD.m_iInternalHandle == kInvalidID_U32) ? g_pMaterialSystemInternal->GetDefaultMaterialReference() : pGeometry.hMaterial_OLD;

	m_RendererBackend->DrawGeometry(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, pGeometry, pMaterial);
}

bool Renderer::CreateTempGeometry()
{
	GeometryConfig_t cubeConfig = MeshGenerator::CreateDefaultCubeConfiguration({50, 0, 50}, 20, 20, 20);
	GeometryConfig_t cubeConfig2 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, 50 }, 20, 20, 20);
	GeometryConfig_t cubeConfig3 = MeshGenerator::CreateDefaultCubeConfiguration({ 60, 40, 0 }, 32, 32, 32);

	if (!m_RendererBackend->AcquireGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, cubeConfig, m_Cube1))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}

	if (!m_RendererBackend->AcquireGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, cubeConfig2, m_Cube2))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}
	
	if (!m_RendererBackend->AcquireGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, cubeConfig3, m_Cube3))
	{
		LOG_ERROR("Failed to acquire geometry resources.");
		return false;
	}

	// Acquire materials
	//m_Cube1.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial");
	//m_Cube2.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial");
	//m_Cube2.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial2");
	//m_Cube3.hMaterial = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial3");

	// TEMP SHADER TEST CODE
	//testShaderHandle = g_pShaderSystem->AcquireShader("CoreShader");
	// END TEMP

	// Load a test model.
	//LOG_DEBUG("Bytes allocated before model loading:");
	//g_pMemoryManager->PrintMemoryStats();
	//m_SponzaModel = g_pOBJLoader->CreateConfigFromFile("Assets/Models/a4_c17_water_tower.obj");
	//m_SponzaModel = g_pOBJLoader->CreateConfigFromFile("Assets/Models/water_tower_test.obj");
	//m_SponzaModel = g_pOBJLoader->CreateConfigFromFile("Assets/Models/combine_soldier.obj");
	//LOG_DEBUG("Bytes allocated after model loading:");

	// Font test.
	//FontHandle_t testFont = g_pFontSystem->AcquireFontResources("Assets/Fonts/arial.ttf");

	// Acquire geometry resources for this text.
	//s_FontTest.SetText("Some Test Text!");
	//m_SponzaModel = g_pOBJLoader->CreateConfigFromFile("Assets/Models/Sponza.obj");

	// Test mesh.
	MeshLoader::MeshResource_t sponzaResource{};
	MeshLoader::LoadMeshResource("Assets/Models/sponza.obj", sponzaResource);
	m_SponzaModel.m_Geometries.Resize(sponzaResource.pObjects.Count());

	// TEMP MATERIALS
	MaterialHandle_t tempMaterial = g_pMaterialSystemInternal->AcquireMaterial("testmaterial3");

	// Load any needed materials.
	LOG_DEBUG("Acquiring material resources for sponza model.");
	for (int idx = 0; idx < sponzaResource.pMaterials.Count(); idx++)
	{
		LOG_VERBOSE("Acquiring material '%s'.", sponzaResource.pMaterials[idx].pszMaterialName);
		g_pMaterialSystemInternal->AcquireMaterialFromResource(sponzaResource.pMaterials[idx]);
	}

	LOG_DEBUG("Acquiring geometry resources for sponza model.");
	for (int idx = 0; idx < sponzaResource.pObjects.Count(); idx++)
	{
		for (int gpIdx = 0; gpIdx < sponzaResource.pObjects[idx]->pGroups.Count(); gpIdx++)
		{
			GeometryConfig_t newConfig{};
			newConfig.Vertices = sponzaResource.pObjects[idx]->pGroups[gpIdx]->Vertices;
			LOG_VERBOSE("Acquiring geometry resources for sub-mesh '%s'.", sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszDebugName);
			if (newConfig.Vertices.Count() > 0)
			{
				GeometryInstance_t newGeometry{};
				g_pRenderer->AcquireGeometryResources(newConfig, newGeometry);

				// Set the material as well.
				newGeometry.hMaterial = g_pMaterialSystemInternal->AcquireMaterial(sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszMaterialName);
				newGeometry.bIsTransparent = g_pMaterialSystemInternal->IsMaterialTransparent(m_SponzaModel.m_Geometries[idx].hMaterial);

				m_SponzaModel.m_Geometries.PushBack(newGeometry);
			}
		}
	}
	// TEMP!
	s_bLevelGeometryLoaded = true;
	// End test mesh.

	return true;
}

void Renderer::DestroyTempGeometry()
{
	for (int idx = 0; idx < m_SponzaModel.m_Geometries.Count(); idx++)
	{
		m_RendererBackend->ReleaseGeometryResources(m_WorldVertexBuffer, m_WorldIndexBuffer, m_SponzaModel.m_Geometries[idx]);
	}
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
	if (*data.c == 'T' && s_Renderer.m_Cube2.hMaterial_OLD.m_iInternalHandle == kInvalidID_U32)
	{
		LOG_DEBUG("Loading test material.");
		s_Renderer.m_Cube2.hMaterial_OLD = g_pMaterialSystemInternal->AcquireMaterialReference("Assets/Materials/testmaterial2");
	}

	// Let other listeners handle this event.
	return false;
}