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
#include "Renderer/DebugRenderer.h"
#include "Resources/GeometryGenerator.h"
#include <Resources/Loaders/MeshLoader.h>
#include "Resources/Map.h"
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
ShaderHandle_t	hUIShader{ kInvalidID_U32 };

// DEBUGTODO: Just a test.
static ShaderHandle_t hDebugLineShader{ kInvalidID_U32 };
struct DebugShaderUniformLocations
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
};

static DebugShaderUniformLocations s_DebugShaderUniforms{};
static unsigned int s_DebugInstanceID{ kInvalidID_U32 };

static GeometryInstance_t debugInstance{};
// DEBUGTODO: End test.

static const char* szRendererVisualizationModesLabels[9] =
{
	"FULLBRIGHT",
	"LIGHTING ONLY",
	"DIFFUSE LIGHTING",
	"SPECULAR LIGHTING",
	"NORMAL MAPS - TANGENT-SPACE",
	"NORMAL MAPS - WORLD SPACE-SPACE",
	"GEOMETRIC NORMALS",
	"GEOMETRIC TANGENTS",
	"GEOMETRIC BITANGENTS",
};

static bool s_bDisplayProfilingText = false;
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
	// 3D Scene View.
	{
		RendererViewConfig_t sceneViewConfig{};
		sceneViewConfig.m_pszViewName = "3D_Scene_View";

		// This pass has 2 attachments: color and depth.
		RendererViewAttachmentConfig_t colorAttachment{};
		colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
		colorAttachment.bShouldClear = true;	// We do want to clear this target before rendering the scene.

		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
		depthAttachment.bShouldClear = true;

		// Add them to the config.
		sceneViewConfig.attachmentConfigs.Resize(2);
		sceneViewConfig.attachmentConfigs.PushBack(colorAttachment);
		sceneViewConfig.attachmentConfigs.PushBack(depthAttachment);

		m_SceneView.m_debuggingColor = Vector3D{ 0.3f, 0.0f, 0.7f };

		m_RendererBackend->CreateRendererViewInternal(sceneViewConfig, m_SceneView);
	}

	// UI View.
	{
		RendererViewConfig_t UIViewConfig{};
		UIViewConfig.m_pszViewName = "UI_View";

		// This pass has 1 attachment: color.
		RendererViewAttachmentConfig_t colorAttachment{};
		colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;
		colorAttachment.bShouldClear = false;

		// TEMP: Adding in a depth pass for now because of renderpass compatibility reasons.
		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;
		depthAttachment.bShouldClear = true;

		UIViewConfig.attachmentConfigs.PushBack(colorAttachment);
		UIViewConfig.attachmentConfigs.PushBack(depthAttachment);

		m_UIView.m_debuggingColor = Vector3D{ 0.7f, 0.0f, 0.9f };

		m_RendererBackend->CreateRendererViewInternal(UIViewConfig, m_UIView);
	}

	// Set up our renderbuffers
	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex3D) * 5 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "World_Vertex_Buffer", m_WorldVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 5 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "World_Index_Buffer", m_WorldIndexBuffer);

	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex2D) * 1 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "UI_Vertex_Buffer", m_UIVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 1 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "UI_Index_Buffer", m_UIIndexBuffer);

	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vector3D) * 1 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Geometry_Buffer", m_DebugRendererBuffer);

	g_pEngineEvents_Internal->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);
	g_pEngineEvents_Internal->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);

	// Grab window dimensions.
	pConfig.pMainWindow->GetWindowSize(m_iWindowWidth, m_iWindowHeight);

	// Setup our internal framebuffers.
	if (!m_RendererBackend->CreateSwapchainFramebuffers(m_SceneView))
	{
		LOG_ERROR("Failed to create swapchain framebuffers. Renderer initialization failed!");
		return false;
	}

	// Setup the debug renderer.
	if (!g_pDebugRenderer->Initialize())
	{
		LOG_ERROR("Failed to initialize the debug renderer.");
		return false;
	}

	s_MainCamera.RecalculateCameraVectors();

	// Start the clock now and we'll take the delta time each frame.
	m_RendererClock.Start();
	m_Metrics.TimeSinceLastMetricsUpdate = m_RendererClock.GetElapsedTime();
	m_Metrics.PreviousFrameFPS = 0;
	m_Metrics.CurrentFrameFPS = 0;

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

	// Shutdown the debug renderer.
	g_pDebugRenderer->Shutdown();

	m_RendererBackend->DestroyRenderBufferInternal(m_DebugRendererBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_UIIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_UIVertexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldVertexBuffer);

	m_RendererBackend->DestroyRendererViewInternal(m_UIView);
	m_RendererBackend->DestroyRendererViewInternal(m_SceneView);

	m_RendererBackend->RendererBackend_Shutdown();

	m_RendererBackend = nullptr;

	// Release any heap allocations
	m_SponzaModel.m_Geometries.Clear();
}

//-------------------------------------------------------------------------
// Purpose: Draws a frame with the renderer.
// Input: pRenderData - Frame-specific data to be used in rendering ops.
//		  pGeometries - An array of geometries to be rendered.
// Output: bool - True on successful frame; if there were any rendering errors, false.
//-------------------------------------------------------------------------
bool Renderer::Update(RenderPacket_t& pRenderData, CMap& pCurrentMap)
{
	// If we are currently minimized, do not do anything!
	if (m_bMinimized)
	{
		return true;
	}

	if (m_Metrics.TimeSinceLastMetricsUpdate + 1 < m_RendererClock.GetElapsedTime())
	{
		// Time to update metrics.
		m_Metrics.PreviousFrameFPS = m_Metrics.CurrentFrameFPS;
		m_Metrics.CurrentFrameFPS = 0;
		m_Metrics.TimeSinceLastMetricsUpdate = m_RendererClock.GetElapsedTime();
	}

	// Clear out the previous frame's metrics.
	m_Metrics.FrameTime = 0.0f;
	m_Metrics.NumDrawCalls = 0;
	m_Metrics.NumGeometriesFrustumCulled = 0;

	// Calculate our delta time.
	double currentFrameTime = m_RendererClock.GetElapsedTime();
	pRenderData.DeltaTime = currentFrameTime - m_flLastFrameTime;
	m_flLastFrameTime = currentFrameTime;

	// Save off the frametime.
	m_Metrics.FrameTime = pRenderData.DeltaTime;

	s_MainCamera.UpdateCameraData(pRenderData.DeltaTime);

	// Draw the frame!
	if (!m_RendererBackend->BeginFrame(pRenderData))
	{
		return false;
	}

	m_RendererBackend->BeginRenderpass(m_SceneView);

	Matrix4x4 matProjection = Matrix4x4::Perspective(M_DEG2RAD_F(75.0f), (float)m_iWindowWidth / m_iWindowHeight, 0.1f, 10000.0f);
	// Flip the y-axis.
	matProjection.flData[5] *= -1;
	Matrix4x4 matView = s_MainCamera.GetViewMatrix();
	Vector vecCameraPos = s_MainCamera.vecPosition;
	Matrix4x4 model = Matrix4x4{ 1.0f };

	g_pMaterialSystemInternal->ApplyGlobalRenderData(matProjection, matView, vecCameraPos, s_Renderer.m_iRendererDebugMode);
	Utl::CVector<Matrix4x4>				TransformMatrices(32);

	if (pCurrentMap.IsMapLoaded())
	{
		// Get the frustum from the camera.
		Frustum frustum = s_MainCamera.CreateFrustumFromCamera();

		// TODO: Make this work with an allocator.
		Utl::CVector<GeometryInstance_t*>	pOpaqueGeometries(512);
		Utl::CVector<GeometryInstance_t*>	pTransparentGeometries(512);
		unsigned int						pCulledGeometries{ 0 };

		// Get the renderables from the map.
		pCurrentMap.GetRenderablesFromScene(frustum, TransformMatrices, pOpaqueGeometries, pTransparentGeometries, &pCulledGeometries);
		m_Metrics.NumGeometriesFrustumCulled = pCulledGeometries;

		for (int idx = 0; idx < pOpaqueGeometries.Count(); idx++)
		{
			// Apply transformations.
			model = TransformMatrices[pOpaqueGeometries[idx]->iLocalToWorldTransform];
			g_pMaterialSystemInternal->ApplyPerDrawRenderData(model);

			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pOpaqueGeometries[idx]->hMaterial);
			DrawGeometry(*pOpaqueGeometries[idx], &m_WorldVertexBuffer, &m_WorldIndexBuffer);

			m_Metrics.NumDrawCalls++;
		}

		// Render the sorted transparent geometries.
		for (int idx = 0; idx < pTransparentGeometries.Count(); idx++)
		{
			// Apply transformations.
			model = TransformMatrices[pTransparentGeometries[idx]->iLocalToWorldTransform];
			g_pMaterialSystemInternal->ApplyPerDrawRenderData(model);

			// Bind the appropriate material.
			g_pMaterialSystemInternal->ApplyMaterialInstance(pTransparentGeometries[idx]->hMaterial);
			DrawGeometry(*pTransparentGeometries[idx], &m_WorldVertexBuffer, &m_WorldIndexBuffer);

			m_Metrics.NumDrawCalls++;
		}

		unsigned int iDebugGeoIndex = kInvalidID_U32;
		static bool bDebugGeoLoaded = false;
		for (int idx = 0; idx < pOpaqueGeometries.Count(); idx++)
		{
			//if (pOpaqueGeometries[idx]->hMaterial == 2)
			//if (pOpaqueGeometries[idx]->hMaterial == 28)
			if (pOpaqueGeometries[idx]->m_MeshID == 3)
			{
				iDebugGeoIndex = idx;
				break;
			}
		}
		if (pCurrentMap.IsMapLoaded() && !bDebugGeoLoaded)
		{
			if (iDebugGeoIndex != kInvalidID_U32)
			{
				Vector4D vecGreen{ 0.0f, 1.0f, 0.0f, 1.0f };
				g_pDebugRenderer->AddBBox(*pOpaqueGeometries[iDebugGeoIndex], vecGreen);
				bDebugGeoLoaded = true;
			}
		}
		else if (pCurrentMap.IsMapLoaded())
		{
			g_pDebugRenderer->AddCameraFrustum(s_MainCamera, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		// DEBUGTODO: Just a test.
		// DEBUGTODO: Create a separate renderpass for debug geometries.
		if (hDebugLineShader != kInvalidID_U32 && debugInstance.vertexCount != kInvalidID_U64 && iDebugGeoIndex != kInvalidID_U32)
		{
			g_pShaderSystem->BindShader(hDebugLineShader);
			model = Matrix4x4{ 1.0f };
			model = TransformMatrices[pOpaqueGeometries[iDebugGeoIndex]->iLocalToWorldTransform];
			g_pShaderSystem->SetShaderUniform(hDebugLineShader, s_DebugShaderUniforms.projection, 0, s_DebugInstanceID, &matProjection);
			g_pShaderSystem->SetShaderUniform(hDebugLineShader, s_DebugShaderUniforms.view, 0, s_DebugInstanceID, &matView);

			g_pShaderSystem->BindPerInstanceUniforms(s_DebugInstanceID);

			Vector4D vecColor{ 0.0f, 1.0f, 0.0f, 1.0f };
			g_pShaderSystem->SetShaderUniform(hDebugLineShader, s_DebugShaderUniforms.color, 0, s_DebugInstanceID, &vecColor);

			g_pShaderSystem->SetShaderUniform(hDebugLineShader, s_DebugShaderUniforms.model, 0, s_DebugInstanceID, &model);
			DrawGeometry(debugInstance, &m_DebugRendererBuffer, &m_DebugRendererBuffer);
		}
	}

	m_RendererBackend->EndRenderpass();

	// Render our debug geometry.
	DebugRendererFrameData_t debugFrameData{};
	debugFrameData.matProjection = matProjection;
	debugFrameData.matView = matView;
	debugFrameData.vecCameraPosition = s_MainCamera.vecPosition;
	debugFrameData.vecCameraForward = s_MainCamera.vecFront;
	debugFrameData.pTransformMatrices = TransformMatrices.Data();
	g_pDebugRenderer->Render(debugFrameData);

	if (testText.m_pszText != nullptr && s_bDisplayProfilingText)
	{
		m_RendererBackend->BeginRenderpass(m_UIView);

		// Render some test UI text.
		Matrix4x4 UIProjection = Matrix4x4::Orthographic(0.0f, (float)m_iWindowWidth, 0.0f, (float)m_iWindowHeight, 0.0f, 10000.0f);
		g_pShaderSystem->SetShaderUniform(1, 0, 0, 1, &UIProjection);
		g_pShaderSystem->BindShader(hUIShader);
		g_pShaderSystem->BindPerInstanceUniforms(1);
		DrawGeometry(testText.m_Geometry, &m_UIVertexBuffer, &m_UIIndexBuffer);

		m_RendererBackend->EndRenderpass();
	}

	m_RendererBackend->EndFrame(pRenderData);
	m_Metrics.CurrentFrameFPS++;

	// Clear out bound shader state.
	g_pShaderSystem->BindShader(kInvalidID_U32);

	if (s_bDisplayProfilingText)
	{
		// Display renderer matrics.
		char szMetricsString[512]{};
		Utl::StringFormat_V("Frametime: %.5f s (FPS: %d)\nDraw calls: %d (Geometry culled: %d)\nCurrent memory pool usage: %.5f\nCurrent map: '%s'\nCurrent Vis Mode: %s",
			szMetricsString,
			m_Metrics.FrameTime, m_Metrics.PreviousFrameFPS,
			m_Metrics.NumDrawCalls,	m_Metrics.NumGeometriesFrustumCulled,
			g_pMemoryManager->GetPercentAllocated(),
			pCurrentMap.GetMapName() != nullptr ? pCurrentMap.GetMapName() : "None",
			m_iRendererDebugMode > 0 ? szRendererVisualizationModesLabels[m_iRendererDebugMode - 1] : "Default");

		// Set the string.
		testText.SetText(szMetricsString);
	}

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
//			pszResourceName - Name to tag the shader resources with. Only used in debug builds.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::AcquireShaderInstanceResources(Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName)
{
	m_RendererBackend->AcquireShaderInstanceResources(pShader, pOutInstanceID, pszResourceName);
}

//-------------------------------------------------------------------------
// Purpose: Releases instance-level resources for a shader.
// Input:	pShader - The shader to get release an instance for.
//			pInstanceID - The instance to be released.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::ReleaseShaderInstanceResources(Shader* pShader, unsigned int pInstanceID)
{
	m_RendererBackend->ReleaseShaderInstanceResources(pShader, pInstanceID);
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
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
bool Renderer::CreateRendererView(RendererViewConfig_t& pViewConfig, RendererView& pOutView)
{
	return m_RendererBackend->CreateRendererViewInternal(pViewConfig, pOutView);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
void Renderer::DestroyRendererView(RendererView& pView)
{
	m_RendererBackend->DestroyRendererViewInternal(pView);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
bool Renderer::BeginRendererView(RendererView& pView)
{
	return m_RendererBackend->BeginRenderpass(pView);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
void Renderer::EndRendererView()
{
	m_RendererBackend->EndRenderpass();
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
bool Renderer::CreateRenderBuffer(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer)
{
	return m_RendererBackend->CreateRenderBufferInternal(bufferSize, bufferUsage, bUseFreelist, pszBufferName, pOutBuffer);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
void Renderer::DestroyRenderBuffer(RenderBuffer& pBuffer)
{
	m_RendererBackend->DestroyRenderBufferInternal(pBuffer);
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
// Input:	pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
//			pGeometryConfig - The specific geometry data.
//			pOutInstance - A structure that holds the instance of geometry after acquisition.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(pVertexBuffer, pIndexBuffer, pGeometryConfig.Vertices.Count(), pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data(), pGeometryConfig.Indices.Count(), pGeometryConfig.Indices.Data(), pOutInstance);
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
	m_RendererBackend->AcquireGeometryResources(&pVertexBuffer, &pIndexBuffer, pGeometryConfig.Vertices.Count(), pGeometryConfig.Vertices.Stride(), pGeometryConfig.Vertices.Data(), pGeometryConfig.Indices.Count(), pGeometryConfig.Indices.Data(), pOutInstance);
}

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
// Just a straight passthrough.
//-------------------------------------------------------------------------
void Renderer::AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->AcquireGeometryResources(pVertexBuffer, pIndexBuffer, iVertexCount, iVertexStride, pVertices, iIndexCount, pIndices, pOutInstance);
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
void Renderer::DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer)
{
	m_RendererBackend->DrawGeometry(pVertexBuffer, pIndexBuffer, pGeometry);
}

//-------------------------------------------------------------------------
// Purpose: Updates the vertex/index data associated with a piece of geometry.
// Input:	pVertexBuffer - The vertex buffer associated with this geometry.
//			pIndexBuffer - The index buffer associated with this geometry.
//			pGeometry - The geometry instance to be updated.
//			pNewGeometryConfig - The new geometry data to overwrite the existing data.
// Output:	none.
//-------------------------------------------------------------------------
void Renderer::UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig) 
{
	m_RendererBackend->UpdateGeometryResources(pVertexBuffer, pIndexBuffer, pGeometry, pNewGeometryConfig);
}

//-------------------------------------------------------------------------
// Purpose: Updates the vertex/index data associated with a piece of geometry.
// Input:	pVertexBuffer - The vertex buffer associated with this geometry.
//			pIndexBuffer - The index buffer associated with this geometry.
//			pGeometry - The geometry instance to be updated.
//			pNewGeometryConfig - The new geometry data to overwrite the existing data.
// Output:	none.
// 
// NOTE: This method handles 2D geometry
// TODO: Find a way to combine this and the above method.
//-------------------------------------------------------------------------
void Renderer::UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig)
{
	m_RendererBackend->UpdateGeometryResources(m_UIVertexBuffer, m_UIIndexBuffer, pGeometry, pNewGeometryConfig);
}

//-------------------------------------------------------------------------
// Purpose: Updates the vertex/index data associated with a piece of geometry.
// Input:	pVertexBuffer - The vertex buffer associated with this geometry.
//			pIndexBuffer - The index buffer associated with this geometry.
//			pGeometry - The geometry instance to be updated.
//			pNewGeometryConfig - The new geometry data to overwrite the existing data.
// Output:	none.
// 
// TODO: Find a way to combine this and the above method.
//-------------------------------------------------------------------------
void Renderer::UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->UpdateGeometryResources(pVertexBuffer, pIndexBuffer, iVertexCount, iVertexStride, pVertices, iIndexCount, pIndices, pOutInstance);
}

//-------------------------------------------------------------------------
// Purpose: A temporary function to create geometry for testing.
// Input:	none.
// Output:	True on successful geometry creation, otherwise false.
// DEBUGTODO:	Find a better name/place for this function.
//				This really is responsible for creating/initializing structures
//				that need to be created post system-initialization.
//-------------------------------------------------------------------------
bool Renderer::CreateTempTextObjects()
{
	hUIShader = g_pShaderSystem->AcquireShader("Core.UI");
	testFont = g_pFontSystem->AcquireFontResources("Assets/Fonts/UbuntuMono-R.ttf");
	testText.m_hFont = testFont;
	testText.m_flTextSize = 1.0f;
	testText.m_vecPos = { -0.95f, -0.6f };	// TODO: Make this screenspace instead of NDC!

	if (!g_pDebugRenderer->InitializeDebugShaders())
	{
		LOG_ERROR("Failed to initialize debug shaders!");
		return false;
	}

	// DEBUGTODO: Just a test.
	{
		hDebugLineShader = g_pShaderSystem->AcquireShader("Debug.Line");

		s_DebugShaderUniforms.projection = g_pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "projection");
		s_DebugShaderUniforms.view = g_pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "view");
		s_DebugShaderUniforms.color = g_pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "color");
		s_DebugShaderUniforms.model = g_pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "model");

		g_pShaderSystem->AcquireShaderInstanceResources(hDebugLineShader, &s_DebugInstanceID, "Test_Debug_Geometry");

		/*Vector3D_OLD positions[6]{};
		positions[0] = Vector3D_OLD{ 0.0f, 0.0f, 0.0f };
		positions[1] = Vector3D_OLD{ 100.0f, 0.0f, 0.0f };
		positions[2] = Vector3D_OLD{ 0.0f, 0.0f, 0.0f };
		positions[3] = Vector3D_OLD{ 0.0f, 100.0f, 0.0f };
		positions[4] = Vector3D_OLD{ 0.0f, 0.0f, 0.0f };
		positions[5] = Vector3D_OLD{ 0.0f, 0.0f, 100.0f };

		m_RendererBackend->AcquireGeometryResources(&m_DebugRendererBuffer, nullptr, 6, sizeof(Vector3D_OLD), positions, 0, nullptr, debugInstance);*/
	}

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

	// Cache off the new dimensions.
	s_Renderer.SetWindowDimensions(data.u32[0], data.u32[1]);

	// Make sure to resize the camera's aspect ratio as well.
	s_MainCamera.flAspectRatio = ((float)data.u32[0] / data.u32[1]);

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

	if (data.c[0] == 'Y')
	{
		// Toggle profiling text.
		s_bDisplayProfilingText = !s_bDisplayProfilingText;
	}

	if (data.c[0] >= 0 && data.c[0] <= 9)
	{
		g_pRenderer->m_iRendererDebugMode = data.c[0];
	}

	// Let other listeners handle this event.
	return false;
}