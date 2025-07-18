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

// TEMP Test font
FontHandle_t	testFont;
TextObject		testText;
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
	m_RendererBackend->CreateRendererViewInternal("3D_Scene_View", m_SceneView);

	m_UIView.m_bShouldClear = false;
	m_RendererBackend->CreateRendererViewInternal("UI_View", m_UIView);

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

	// Save off the frametime.
	m_Metrics.FrameTime = pRenderData.DeltaTime;

	s_MainCamera.UpdateCameraData(pRenderData.DeltaTime);

	// Update renderer matrics.
	char szMetricsString[512]{};
	StringFormat_V("Frametime: %.5f ms", szMetricsString, m_Metrics.FrameTime);
	// Set the string.
	testText.SetText(szMetricsString);

	// Draw the frame!
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
	Matrix4x4 model = Matrix4x4::Matrix4x4(1.0f);

	// TODO: Get these shader locations set in here.
	// TODO: Don't hardcode these things here
	g_pShaderSystem->SetShaderUniform(0, 0, 0, 0, &projection);
	g_pShaderSystem->SetShaderUniform(0, 1, 0, 0, &view);
	g_pShaderSystem->SetShaderUniform(0, 2, 0, 0, &myPos);
	g_pShaderSystem->SetShaderUniform(0, 13, 0, 0, &model);

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
		g_pMaterialSystemInternal->ApplyMaterialInstance(m_Cube3.hMaterial);
		DrawGeometry(m_Cube3, m_WorldVertexBuffer, m_WorldIndexBuffer);

		// Render the opaque geometry first.
		for (int idx = 0; idx < pOpaqueGeometries.Count(); idx++)
		{
			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pOpaqueGeometries[idx]->hMaterial);
			DrawGeometry(*pOpaqueGeometries[idx], m_WorldVertexBuffer, m_WorldIndexBuffer);
		}

		// TODO: Sort the transparent geometries from farthest to closest to the camera.

		// Render the sorted transparent geometries.
		for (int idx = 0; idx < pTransparentGeometries.Count(); idx++)
		{
			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pTransparentGeometries[idx]->hMaterial);
			DrawGeometry(*pTransparentGeometries[idx], m_WorldVertexBuffer, m_WorldIndexBuffer);
		}
	}

	m_RendererBackend->EndRenderpass();

	if (testText.m_pszText != nullptr)
	{
		m_RendererBackend->StartRenderpass(m_UIView);

		// Render some test UI text.
		Matrix4x4 UIProjection = Matrix4x4::Ortho(0.0f, 800.0f, 600.0f, 0.0f, 0.0f, 10000.0f);
		g_pShaderSystem->SetShaderUniform(1, 0, 0, 1, &UIProjection);
		g_pShaderSystem->BindShader(1);	// UI Shader TODO: Don't hardcode.
		g_pShaderSystem->BindPerInstanceUniforms(1);
		DrawGeometry(testText.m_Geometry, m_UIVertexBuffer, m_UIIndexBuffer);

		m_RendererBackend->EndRenderpass();
	}

	m_RendererBackend->EndFrame(pRenderData);

	// Increment the frame count.
	m_iCurrentRendererFrame++;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Acquires renderer-specific resources for a texture.
// Input:	pTexture - The texture to get resources for.
//			internalIndex - The internal handle for this texture.
//			nPixelDataSize - The size in bytes of the pixel data.
//			pPixelData - An array of the texture's pixel data.
// Output:	bool - Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool Renderer::AcquireTextureResources(Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	return m_RendererBackend->AcquireTextureResources(pTexture, internalIndex, nPixelDataSize, pPixelData);
}

//-------------------------------------------------------------------------
// Purpose: Releases renderer-specific resources for a texture.
// Input:	pTexture - The texture to release resources for.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::ReleaseTextureResources(Texture* pTexture)
{
	m_RendererBackend->ReleaseTextureResources(pTexture);
}

//-------------------------------------------------------------------------
// Purpose: Creates/Acquires the renderer-specific resources for a shader.
// Input:	pShader - The shader to get resources for.
//			pConfig - Some shader configuration details.
// Output:	bool - Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool Renderer::CreateShader(Shader* pShader, ShaderConfig_t& pConfig)
{
	return m_RendererBackend->CreateShader(pShader, pConfig); 
}

//-------------------------------------------------------------------------
// Purpose: Releases renderer-specific resources for a shader.
// Input:	pShader - The shader to release resources for.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::DestroyShader(Shader* pShader)
{
	m_RendererBackend->DestroyShader(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Acquires instance-level resources for a shader.
//				Typically called when setting up a material or similar scenarios.
// Input:	pShader - The shader to get set up an instance for.
//			pOutInstanceID - A pointer to an int to hold the ID of the instance.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::AcquireShaderInstanceResources(Shader* pShader, unsigned int* pOutInstanceID)
{
	m_RendererBackend->AcquireShaderInstanceResources(pShader, pOutInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader for use in rendering operations.
// Input:	pShader - The shader to bind.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::BindShader(Shader* pShader)
{
	m_RendererBackend->BindShader(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader's per-frame uniforms.
// Input:	pShader - The shader who's uniforms to bind.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::BindShaderPerFrameUniforms(Shader* pShader)
{
	m_RendererBackend->BindPerFrameShaderDescriptors(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader's per-instance uniforms.
// Input:	pShader - The shader who's uniforms to bind.
//			hInstanceID - The instance to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::BindShaderPerInstanceUniforms(Shader* pShader, unsigned int hInstanceID)
{
	m_RendererBackend->BindPerInstanceShaderDescriptors(pShader, hInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Sets a shader's uniform.
// Input:	pShader - The shader who's uniform is to be set.
//			pUniform - A pointer to the uniform to be set.
//			iUniformArrayIndex - The array index of the uniform to be set.
//			hInstanceID - The instance to be set.
//			pData - An opaque pointer to the value to be set in the uniform.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::SetShaderUniform(Shader* pShader, ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData)
{
	m_RendererBackend->SetShaderUniform(pShader, pUniform, iUniformArrayIndex, hInstanceID, pData);
}

//-------------------------------------------------------------------------
// Purpose: Acquires the renderer-specific resources for rendering geometry.
// Input:	pGeometryConfig - The specific geometry data.
//			pOutInstance - A structure that holds the instance of geometry after acquisition.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(g_pRenderer->m_WorldVertexBuffer, g_pRenderer->m_WorldIndexBuffer, pGeometryConfig, pOutInstance);
}

//-------------------------------------------------------------------------
// Purpose: Acquires the renderer-specific resources for rendering geometry.
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometryConfig - The specific geometry data.
//			pOutInstance - A structure that holds the instance of geometry after acquisition.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(pVertexBuffer, pIndexBuffer, pGeometryConfig, pOutInstance);
}

//-------------------------------------------------------------------------
// Purpose: Releases the renderer-specific resources for geometry.
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometry - The geometry instance to be released.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry)
{
	m_RendererBackend->ReleaseGeometryResources(pVertexBuffer, pIndexBuffer, pGeometry);
}

//-------------------------------------------------------------------------
// Purpose: Renders a piece of geometry (MUST BE CALLED DURING THE RENDERING OF A FRAME!!)
// Input:	pGeometry - The geometry instance to be drawn.
//			pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer)
{
	m_RendererBackend->DrawGeometry(pVertexBuffer, pIndexBuffer, pGeometry);
}

//-------------------------------------------------------------------------
// Purpose: A temporary function to create geometry for testing.
// Input:	none.
// Output:	True on successful geometry creation, otherwise false.
//-------------------------------------------------------------------------
bool Renderer::CreateTempGeometry()
{
	GeometryConfig_t cubeConfig = MeshGenerator::CreateDefaultCubeConfiguration({50, 0, 50}, 20, 20, 20);
	GeometryConfig_t cubeConfig2 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 0, 50 }, 20, 20, 20);
	GeometryConfig_t cubeConfig3 = MeshGenerator::CreateDefaultCubeConfiguration({ 0, 30, 0 }, 32, 32, 32);

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
	m_Cube3.hMaterial = g_pMaterialSystemInternal->AcquireMaterial("testmaterial3");

	MeshLoader::MeshResource_t sponzaResource{};
	MeshLoader::LoadMeshResource("Assets/Models/sponza.cmodel_c", sponzaResource);
	//MeshLoader::LoadMeshResource("Assets/Models/interior.cmodel_c", sponzaResource);

	// Test mesh.
	//MeshLoader::MeshResource_t sponzaResource{};
	//MeshLoader::LoadMeshResource("Assets/Models/sponza.obj", sponzaResource);
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
			newConfig.Indices = sponzaResource.pObjects[idx]->pGroups[gpIdx]->Indices;
			if (sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszDebugName)
				newConfig.pszDebugName = StringDuplicate(sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszDebugName);
			LOG_VERBOSE("Acquiring geometry resources for sub-mesh '%s'.", sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszDebugName);
			if (newConfig.Vertices.Count() > 0)
			{
				GeometryInstance_t newGeometry{};
				g_pRenderer->AcquireGeometryResources(newConfig, newGeometry);

				// Set the material as well.
				newGeometry.hMaterial = g_pMaterialSystemInternal->AcquireMaterial(sponzaResource.pObjects[idx]->pGroups[gpIdx]->pszMaterialName);
				newGeometry.bIsTransparent = g_pMaterialSystemInternal->IsMaterialTransparent(newGeometry.hMaterial);

				m_SponzaModel.m_Geometries.PushBack(newGeometry);
			}
		}
	}
	// TEMP!
	s_bLevelGeometryLoaded = true;
	// End test mesh.

	// TEMP TEST FONT
	testFont = g_pFontSystem->AcquireFontResources("Assets/Fonts/UbuntuMono-R.ttf");
	//testText.SetText("Some test text!");
	// END TEMP

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases all temp geometry resources.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::DestroyTempGeometry()
{
	for (int idx = 0; idx < m_SponzaModel.m_Geometries.Count(); idx++)
	{
		m_RendererBackend->ReleaseGeometryResources(m_WorldVertexBuffer, m_WorldIndexBuffer, m_SponzaModel.m_Geometries[idx]);
	}
}

//-------------------------------------------------------------------------
// Purpose: An event handler for window resizes.
// Input:	data - Event-specific data (this contains the new dimensions of the window).
// Output:	Returns true if this event should be considered 'handled', false if it should propogate to other listeners.
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// Purpose: An event handler for key pressed.
// Input:	data - Event-specific data (this contains the relevant pressed key-codes).
// Output:	Returns true if this event should be considered 'handled', false if it should propogate to other listeners.
//-------------------------------------------------------------------------
static bool RendererEvent_KeyPressed(EventData_t data)
{
	if (data.c[0] == 'P')
	{
		// Print the camera's position and angles.
		LOG_INFO("Pos: %f, %f, %f\tAng: %f, %f", s_MainCamera.vecPosition.x, s_MainCamera.vecPosition.y, s_MainCamera.vecPosition.z, s_MainCamera.Pitch, s_MainCamera.Yaw);
	}

	if (data.c[0] >= 0  && data.c[0] <= 9)
	{
		g_pRenderer->m_iRendererDebugMode = data.c[0];

		switch (data.c[0])
		{
		default:
		case 0:
			g_pRenderer->m_iRendererDebugMode = 0;
			LOG_INFO("Visualization mode set to default.");
			break;
		case 1:
			LOG_INFO("Visualization mode set to fullbright.");
			break;
		case 2:
			LOG_INFO("Visualization mode set to lighting only.");
			break;
		case 3:
			LOG_INFO("Visualization mode set to diffuse lighting.");
			break;
		case 4:
			LOG_INFO("Visualization mode set to specular lighting.");
			break;
		case 5:
			LOG_INFO("Visualization mode set to normals - tangent space.");
			break;
		case 6:
			LOG_INFO("Visualization mode set to normals - world space.");
			break;
		case 7:
			LOG_INFO("Visualization mode set to geometric normal.");
			break;
		case 8:
			LOG_INFO("Visualization mode set to tangent u - world space.");
			break;
		case 9:
			LOG_INFO("Visualization mode set to tangent v - world space.");
			break;
		}

		g_pShaderSystem->SetShaderUniform(0, 9, 0, 0, &data.c[0]); // TODO: Don't hardcode these things!
	}
	return false;

	// Let other listeners handle this event.
	return false;
}