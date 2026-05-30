//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Renderer Front-end
//=============================================================================
#include "Renderer/Renderer.h"

#include "Core/Engine.h"
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

// Event handlers.
static bool RendererEvent_WindowResized(EventData_t data);
static bool RendererEvent_KeyPressed(EventData_t data);

// A callback to be used to get an opaque pointer to the renderer-backend's state.
typedef void* (*PFN_GetRendererBackendState)();

// TEMP Test font
FontHandle_t	testFont;
TextObject		testText;
ShaderHandle_t	hUIShader{ kInvalidID_U32 };

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
// The renderer frontend.
//-------------------------------------------------------------------------
class CRenderer : public IRenderer
{
public:
	// Initialize, shutdown.
	bool			Initialize(RendererConfig_t& pAppConfig) override;
	void			Shutdown() override;
	bool			Update(Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData, class CMap* pCurrentMap) override;

	bool			RegisterWindow(Platform::IPlatformWindow& pWindow) override;
	void			ReleaseWindowResources(Platform::IPlatformWindow& pWindow) override;

	void			ConfigureRenderViews(Utl::CVector<IRenderView*>& pRenderViews) override;
	void			RegisterRenderView(IRenderView* pView) override;

	bool			CreateRenderpass(struct RenderpassConfig_t* pConfig, struct Renderpass* pRenderpass) override;
	void			DestroyRenderpass(struct Renderpass* pRenderpass) override;
	IRenderView*	GetRenderView(const char* pszRenderpassName) override;

	bool			BeginRenderpass(struct Renderpass* pRenderpass) override;
	void			EndRenderpass() override;

	bool			AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) override;
	void			ReleaseTextureResources(struct Texture* pTexture) override;

	bool			CreateShader(class Shader* pShader, struct ShaderConfig_t& pAppConfig) override;
	void			DestroyShader(class Shader* pShader) override;

	void			AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName) override;
	void			ReleaseShaderInstanceResources(class Shader* pShader, unsigned int pInstanceID) override;

	void			BindShader(class Shader* pShader) override;
	void			BindShaderPerFrameUniforms(class Shader* pShader) override;
	void			BindShaderPerInstanceUniforms(class Shader* pShader, unsigned int hInstanceID) override;

	void			SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) override;

	bool			CreateRenderBuffer(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer) override;
	void			DestroyRenderBuffer(RenderBuffer& pBuffer) override;

	bool			AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, uint64_t iVertexBase, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t* pOutInstance) override;

	void			DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer) override;

	void			UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig) override;
	void			UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig) override;
	void			UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) override;

	const Camera_t& GetMainCamera() const { return m_Camera; }
	virtual float	GetRendererTime() const { return (float)m_flLastFrameTime; }

	// Temp geometry stuff
	bool	CreateTempTextObjects() override;
	void	DestroyTempGeometry() override;

	void	SetMinimized(bool bMinimized) override { m_bMinimized = bMinimized; }
	void	SetWindowDimensions(unsigned int iWidth, unsigned int iHeight) override { m_iWindowWidth = iWidth; m_iWindowHeight = iHeight; }

	virtual RenderBuffer& GetWorldVertexBuffer() override { return m_WorldVertexBuffer; }
	virtual RenderBuffer& GetWorldIndexBuffer() override { return m_WorldIndexBuffer; }

	virtual RenderBuffer& GetUIVertexBuffer() override { return m_UIVertexBuffer; }
	virtual RenderBuffer& GetUIIndexBuffer() override { return m_UIIndexBuffer; }

	virtual RenderBuffer& GetDebugVertexBuffer() override { return m_DebugVertexBuffer; }
	virtual RenderBuffer& GetDebugIndexBuffer() override { return m_DebugIndexBuffer; }

	virtual inline size_t	GetCurrentRenderFrame() const override { return m_iCurrentRendererFrame; }

private:
	RendererConfig_t	m_Config{};
	IRendererBackend*	m_RendererBackend{ nullptr };
	bool				m_bMinimized{ false };
	Clock				m_RendererClock{};
	double				m_flLastFrameTime{};

	RenderMetrics_t		m_Metrics{};

	// The current dimensions of the surface we are presenting to (usually a window).
	unsigned int	m_iWindowWidth{ 0 };
	unsigned int	m_iWindowHeight{ 0 };

	// An array of registered render-views.
	Utl::CVector<IRenderView*>	m_pRenderViews{};

public:
	Camera_t			m_Camera{};

	// TODO: Should these live here or with the render view?
	RenderBuffer		m_WorldVertexBuffer{};
	RenderBuffer		m_WorldIndexBuffer{};

	RenderBuffer		m_UIVertexBuffer{};
	RenderBuffer		m_UIIndexBuffer{};

	RenderBuffer		m_DebugVertexBuffer{};
	RenderBuffer		m_DebugIndexBuffer{};

	size_t				m_iCurrentRendererFrame{ kInvalidID_U64 };
	uint8_t				m_iRendererDebugMode{ 0 };

	// A pointer to the window we're rendering to.
	Platform::IPlatformWindow* m_pMainWindow;

	class IDebugRenderer* m_pDebugRenderer{ nullptr };
};

// Internal pointer to the renderer system.
// Needed for internal code.
// TODO: maybe there's a better way to pass this around?
static CRenderer* s_RenderSystem{ nullptr };

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IRenderer::AllocateRenderSystemState(IRenderer** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold render system state was not provided! Render system creation failed!");
		return;
	}

	s_RenderSystem = new CRenderer;
	*ppSystemState = s_RenderSystem;
}

//-------------------------------------------------------------------------
// Purpose: Initialize the Renderer frontend and backend.
// Input: pConfig - The configuration for both the frontend and backend.
// Output: bool - Returns true on successful initialization, otherwise false.
//-------------------------------------------------------------------------
bool CRenderer::Initialize(RendererConfig_t& pAppConfig)
{
	// Load up the renderer backend module, and connect our interfaces.
	Platform::ISysModule* pVulkanRendererDLL = pAppConfig.pRendererBackendDLL;

	PFN_GetRendererBackendState getRendererBackend = (PFN_GetRendererBackendState)pVulkanRendererDLL->GetFuncAddr("GetRendererBackendState");
	if (!getRendererBackend)
	{
		LOG_ERROR("IRenderer::Initialize failed to get a pointer to the renderer backend. Initialization failed.");
		return false;
	}

	// Get an opaque pointer to the renderer backend.
	m_RendererBackend = (IRendererBackend*)getRendererBackend();

	if (m_RendererBackend != nullptr)
	{
		if (!m_RendererBackend->RendererBackend_Initialize(pAppConfig))
		{
			return false;
		}
	}
	else
	{
		LOG_FATAL("No rendering backend was provided, application cannot continue.");
		return false;
	}

	// Set up our renderbuffers
	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex3D) * 5 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "World_Vertex_Buffer", m_WorldVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 5 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "World_Index_Buffer", m_WorldIndexBuffer);

	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vertex2D) * 1 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "UI_Vertex_Buffer", m_UIVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 1 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "UI_Index_Buffer", m_UIIndexBuffer);

	m_RendererBackend->CreateRenderBufferInternal(sizeof(Vector3D) * 1024, eRenderBufferUsage::VERTEX, true, "Debug_Renderer_Line_Vertex_Buffer", m_DebugVertexBuffer);
	m_RendererBackend->CreateRenderBufferInternal(sizeof(uint32_t) * 1024, eRenderBufferUsage::INDEX, true, "Debug_Renderer_Line_Index_Buffer", m_DebugIndexBuffer);

	if (pAppConfig.pEngine && pAppConfig.pEngine->GetEngineInterfaces()->pEngineEventHandler)
	{
		IEventSystem* pEventHandler = pAppConfig.pEngine->GetEngineInterfaces()->pEngineEventHandler;
		pEventHandler->EventRegister(eEventCategory::WINDOW_RESIZE, nullptr, RendererEvent_WindowResized);
		pEventHandler->EventRegister(eEventCategory::INPUT_KEY_PRESSED, nullptr, RendererEvent_KeyPressed);
	}

	// Grab window dimensions.
	pAppConfig.pMainWindow->GetWindowSize(m_iWindowWidth, m_iWindowHeight);

	// Start the clock now and we'll take the delta time each frame.
	m_RendererClock.Start();
	m_Metrics.TimeSinceLastMetricsUpdate = m_RendererClock.GetElapsedTime();
	m_Metrics.PreviousFrameFPS = 0;
	m_Metrics.CurrentFrameFPS = 0;

	m_iCurrentRendererFrame = 0;

	// Store off a pointer to the window we're rendering to.
	m_pMainWindow = pAppConfig.pMainWindow;

	// Save off the config.
	m_Config = pAppConfig;

	LOG_INFO("Renderer Initialized!");
	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CRenderer::ConfigureRenderViews(Utl::CVector<IRenderView*>& pRenderViews)
{
	for (int iView = 0; iView < pRenderViews.Count(); iView++)
	{
		if (!pRenderViews[iView]->Initialize(m_Config.pEngine->GetEngineInterfaces()))
		{
			LOG_ERROR("Render view failed to initialize!");
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CRenderer::RegisterRenderView(IRenderView* pView)
{
	if (pView)
	{
		m_pRenderViews.PushBack(pView);
	}
}

//-------------------------------------------------------------------------
// Purpose: Shuts down the renderer system.
//-------------------------------------------------------------------------
void CRenderer::Shutdown()
{
	LOG_INFO("Shutting Down Renderer");

	// Release render-views.
	for (int iView = 0; iView < m_pRenderViews.Count(); iView++)
	{
		m_pRenderViews[iView]->Shutdown(this);
	}

	// TODO: Is this the right place to free these guys?
	m_pRenderViews.Clear();

	m_RendererBackend->DestroyRenderBufferInternal(m_DebugIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_DebugVertexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_UIIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_UIVertexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldIndexBuffer);
	m_RendererBackend->DestroyRenderBufferInternal(m_WorldVertexBuffer);

	m_RendererBackend->RendererBackend_Shutdown();

	m_RendererBackend = nullptr;
	s_RenderSystem = nullptr;
}

//-------------------------------------------------------------------------
// Purpose: Draws a frame with the renderer.
// Input: pTargetWindow - The window to render to.
//		  pRenderData - Frame-specific data to be used in rendering ops.
//		  pCurrentMap - The current map to get rendering data from.
// Output: bool - True on successful frame; if there were any rendering errors, false.
//-------------------------------------------------------------------------
bool CRenderer::Update(Platform::IPlatformWindow* pTargetWindow, RenderPacket_t& pRenderData, CMap* pCurrentMap)
{
	// If we are currently minimized, do not do anything!
	if (m_bMinimized)
	{
		return true;
	}
	pRenderData.pMetrics = &m_Metrics;

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
	m_flLastFrameTime = currentFrameTime;

	// TODO: Clear frame data.
	pRenderData.pMetrics->NumDrawCalls = 0;
	pRenderData.pMetrics->NumGeometriesFrustumCulled = 0;

	// Save off the frametime.
	m_Metrics.FrameTime = pRenderData.DeltaTime;

	// Set per-frame render data.
	IShaderSystem* pShaderSystem = m_Config.pEngine->GetEngineInterfaces()->pShaderSystem;
	IMaterialSystem* pMaterialSystem = m_Config.pEngine->GetEngineInterfaces()->pMaterialSystem;

	Matrix4x4 matProjection = m_pMainWindow->GetViewport().matProjection;
	Matrix4x4 matView = pRenderData.pCamera->GetViewMatrix();

	// Flip the y-axis.
	matProjection.flData[5] *= -1.0f;
	Vector vecCameraPos = pRenderData.pCamera->vecPosition;
	Matrix4x4 matModel = Matrix4x4{ 1.0f };

	pRenderData.matProjection = matProjection;

	// View Test TEMP
	// Start rendering!

	// UI TEST
	testText.SetText(this, m_Config.pEngine->GetEngineInterfaces()->pFontSystem, "TESTING!");
	m_RendererBackend->BeginFrame(pTargetWindow, pRenderData);
	{
		// Set per-frame render data.
		pMaterialSystem->ApplyGlobalRenderData(matProjection, matView, vecCameraPos, s_RenderSystem->m_iRendererDebugMode);
		
		for (int iView = 0; iView < m_pRenderViews.Count(); iView++)
		{
			m_pRenderViews[iView]->Render(&pRenderData);
		}
	}
	m_RendererBackend->EndFrame(pTargetWindow, pRenderData);
	// End View Test TEMP

	/*
	// Update debug geometries.
	m_pDebugRenderer->Update();

	// Render debug geometry.
	{
		DebugRendererFrameData_t debugFrameData{};
		debugFrameData.matProjection = matProjection;
		debugFrameData.matView = matView;
		debugFrameData.vecCameraPosition = m_Camera.vecPosition;
		debugFrameData.vecCameraForward = m_Camera.vecFront;
		debugFrameData.flCurTime = (float)m_RendererClock.GetElapsedTime();
		m_pDebugRenderer->Render(debugFrameData);
	}

	m_RendererBackend->EndFrame(pTargetWindow, pRenderData);*/
	
	// Clear out bound shader state.
	// FIXME: We need a way to clear out bound buffer offsets automatically, forgetting to add buffers here keeps biting us.
	pShaderSystem->BindShader(kInvalidID_U32);
	m_WorldVertexBuffer.m_CurrentBoundOffset = kInvalidID_U64;
	m_WorldIndexBuffer.m_CurrentBoundOffset = kInvalidID_U64;
	m_DebugVertexBuffer.m_CurrentBoundOffset = kInvalidID_U64;
	m_DebugIndexBuffer.m_CurrentBoundOffset = kInvalidID_U64;
	m_UIVertexBuffer.m_CurrentBoundOffset = kInvalidID_U64;
	m_UIIndexBuffer.m_CurrentBoundOffset = kInvalidID_U64;

	m_Metrics.CurrentFrameFPS++;

	// Increment the frame count.
	m_iCurrentRendererFrame++;

	return true;
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation.
//-------------------------------------------------------------------------
bool CRenderer::RegisterWindow(Platform::IPlatformWindow& pWindow)
{
	return m_RendererBackend->RegisterWindow(pWindow);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation.
//-------------------------------------------------------------------------
void CRenderer::ReleaseWindowResources(Platform::IPlatformWindow& pWindow)
{
	m_RendererBackend->ReleaseWindowResources(pWindow);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
bool CRenderer::CreateRenderpass(struct RenderpassConfig_t* pConfig, struct Renderpass* pRenderpass)
{
	if (pConfig->attachmentConfigs.Count() < 1)
	{
		LOG_ERROR("Cannot create renderpass without attachments!");
		return false;
	}

	if (pConfig->pszName)
		pRenderpass->pszName = Utl::StringDuplicate(pConfig->pszName);
	
	return m_RendererBackend->CreateRenderpass(pConfig, pRenderpass);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CRenderer::DestroyRenderpass(struct Renderpass* pRenderpass)
{
	if (pRenderpass->pszName)
	{
		Utl::StringFree(pRenderpass->pszName);
		pRenderpass->pszName = nullptr;
	}

	m_RendererBackend->DestroyRenderpass(pRenderpass);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
IRenderView* CRenderer::GetRenderView(const char* pszRenderViewName)
{
	CAssert(pszRenderViewName != nullptr);

	for (int iPass = 0; iPass < m_pRenderViews.Count(); iPass++)
	{
		if (Utl::StringCompareInsensitive(pszRenderViewName, m_pRenderViews[iPass]->GetViewName()))
		{
			return m_pRenderViews[iPass];
		}
	}

	// TODO: nullptr?
	LOG_ERROR("Failed to find renderpass named '%s'. Returning first pass.", pszRenderViewName);
	return m_pRenderViews[0];
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
bool CRenderer::BeginRenderpass(Renderpass* pRenderpass)
{
	CAssert(pRenderpass != nullptr);
	return m_RendererBackend->BeginRenderpass(pRenderpass);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation
//-------------------------------------------------------------------------
void CRenderer::EndRenderpass()
{
	m_RendererBackend->EndRenderpass();
}

//-------------------------------------------------------------------------
// Purpose: Acquires renderer-specific resources for a texture.
// Input:	pTexture - The texture to get resources for.
//			internalIndex - The internal handle for this texture.
//			nPixelDataSize - The size in bytes of the pixel data.
//			pPixelData - An array of the texture's pixel data.
// Output:	bool - Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool CRenderer::AcquireTextureResources(Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData)
{
	return m_RendererBackend->AcquireTextureResources(pTexture, internalIndex, nPixelDataSize, pPixelData);
}

//-------------------------------------------------------------------------
// Purpose: Releases renderer-specific resources for a texture.
// Input:	pTexture - The texture to release resources for.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::ReleaseTextureResources(Texture* pTexture)
{
	m_RendererBackend->ReleaseTextureResources(pTexture);
}

//-------------------------------------------------------------------------
// Purpose: Creates/Acquires the renderer-specific resources for a shader.
// Input:	pShader - The shader to get resources for.
//			pConfig - Some shader configuration details.
// Output:	bool - Returns true on successful resource acquisition, otherwise false.
//-------------------------------------------------------------------------
bool CRenderer::CreateShader(Shader* pShader, ShaderConfig_t& pAppConfig)
{
	return m_RendererBackend->CreateShader(pShader, pAppConfig); 
}

//-------------------------------------------------------------------------
// Purpose: Releases renderer-specific resources for a shader.
// Input:	pShader - The shader to release resources for.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::DestroyShader(Shader* pShader)
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
void CRenderer::AcquireShaderInstanceResources(Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName)
{
	m_RendererBackend->AcquireShaderInstanceResources(pShader, pOutInstanceID, pszResourceName);
}

//-------------------------------------------------------------------------
// Purpose: Releases instance-level resources for a shader.
// Input:	pShader - The shader to get release an instance for.
//			pInstanceID - The instance to be released.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::ReleaseShaderInstanceResources(Shader* pShader, unsigned int pInstanceID)
{
	m_RendererBackend->ReleaseShaderInstanceResources(pShader, pInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader for use in rendering operations.
// Input:	pShader - The shader to bind.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::BindShader(Shader* pShader)
{
	m_RendererBackend->BindShader(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader's per-frame uniforms.
// Input:	pShader - The shader whose uniforms to bind.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::BindShaderPerFrameUniforms(Shader* pShader)
{
	m_RendererBackend->BindPerFrameShaderDescriptors(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Binds a shader's per-instance uniforms.
// Input:	pShader - The shader whose uniforms to bind.
//			hInstanceID - The instance to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::BindShaderPerInstanceUniforms(Shader* pShader, unsigned int hInstanceID)
{
	m_RendererBackend->BindPerInstanceShaderDescriptors(pShader, hInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Creates/acquires the resources needed for a renderbuffer
// Input:	bufferSize - The total size of the buffer in bytes.
//			bufferUsage - Describes what the buffer will be used for.
//			bUseFreelist - Indicates if the buffer should use a freelist for memory tracking.
//						   If this is false, then the buffer allocates/frees memory in a linear fashion.
//			pszBufferName - The name of the buffer, optional and only for debugging purposes.
//			pOutBuffer - Stores the newly created renderbuffer.
// Output:	Returns true on successful creation, otherwise false.
//-------------------------------------------------------------------------
bool CRenderer::CreateRenderBuffer(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer)
{
	bool bCreateResult = m_RendererBackend->CreateRenderBufferInternal(bufferSize, bufferUsage, bUseFreelist, pszBufferName, pOutBuffer);

	return bCreateResult;
}

//-------------------------------------------------------------------------
// Purpose: Destroys/releases the resources used for a render buffer.
// Input:	pBuffer - The buffer to be destroyed.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::DestroyRenderBuffer(RenderBuffer& pBuffer)
{
	m_RendererBackend->DestroyRenderBufferInternal(pBuffer);
}

//-------------------------------------------------------------------------
// Purpose: TODO: Documentation.
//-------------------------------------------------------------------------
bool CRenderer::AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, uint64_t iVertexBase, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t* pOutInstance)
{
	CAssert(iVertexCount > 0 && pVertices != nullptr && pOutInstance != nullptr);

	pOutInstance->vertexCount = iVertexCount;

	pOutInstance->indexCount = iIndexCount;

	m_RendererBackend->AcquireGeometryResources(&pVertexBuffer, &pIndexBuffer, iVertexCount, iVertexStride, iVertexBase, pVertices, iIndexCount, pIndices, *pOutInstance);

	return true;
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
void CRenderer::SetShaderUniform(Shader* pShader, ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData)
{
	m_RendererBackend->SetShaderUniform(pShader, pUniform, iUniformArrayIndex, hInstanceID, pData);
}

//-------------------------------------------------------------------------
// Purpose: Renders a piece of geometry (MUST BE CALLED DURING THE RENDERING OF A FRAME!!)
// Input:	pGeometry - The geometry instance to be drawn.
//			pVertexBuffer - The vertex buffer to be associated with this geometry.
//			pIndexBuffer - The index buffer to be associated with this geometry.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer)
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
void CRenderer::UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig)
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
void CRenderer::UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig)
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
void CRenderer::UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance)
{
	m_RendererBackend->UpdateGeometryResources(pVertexBuffer, pIndexBuffer, iVertexCount, iVertexStride, pVertices, iIndexCount, pIndices, pOutInstance);
}

//-------------------------------------------------------------------------
// Purpose: A temporary function to create geometry for testing.
// Input:	none.
// Output:	True on successful geometry creation, otherwise false.
// TODO:	Find a better name/place for this function.
//				This really is responsible for creating/initializing structures
//				that need to be created post system-initialization.
//-------------------------------------------------------------------------
bool CRenderer::CreateTempTextObjects()
{
	hUIShader = m_Config.pEngine->GetEngineInterfaces()->pShaderSystem->AcquireShader("Core.UI");
	testFont = m_Config.pEngine->GetEngineInterfaces()->pFontSystem->AcquireFontResources("Assets/Fonts/UbuntuMono-R.ttf");
	testText.m_hFont = testFont;
	testText.m_flTextSize = 1.0f;
	testText.m_vecPos = { -0.95f, -0.6f };	// TODO: Make this screenspace instead of NDC!

	m_pDebugRenderer = m_Config.pEngine->GetEngineInterfaces()->pDebugRenderer;
	if (!m_pDebugRenderer->InitializeDebugShaders())
	{
		LOG_ERROR("Failed to initialize debug shaders!");
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Releases all temp geometry resources.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CRenderer::DestroyTempGeometry()
{
	// TODO: Remove me!
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
		s_RenderSystem->SetMinimized(true);
	}
	else
	{
		s_RenderSystem->SetMinimized(false);
	}

	// Cache off the new dimensions.
	s_RenderSystem->SetWindowDimensions(data.u32[0], data.u32[1]);

	// Make sure to resize the camera's aspect ratio as well.
	s_RenderSystem->m_Camera.flAspectRatio = ((float)data.u32[0] / data.u32[1]);

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
		LOG_INFO("Pos: %f, %f, %f\tAng: %f, %f", s_RenderSystem->m_Camera.vecPosition.x, s_RenderSystem->m_Camera.vecPosition.y, s_RenderSystem->m_Camera.vecPosition.z, s_RenderSystem->m_Camera.Pitch, s_RenderSystem->m_Camera.Yaw);
	}

	if (data.c[0] == 'Y')
	{
		// Toggle profiling text.
		s_bDisplayProfilingText = !s_bDisplayProfilingText;
	}

	if (data.c[0] >= 0 && data.c[0] <= 9)
	{
		s_RenderSystem->m_iRendererDebugMode = data.c[0];
	}

	// Let other listeners handle this event.
	return false;
}
