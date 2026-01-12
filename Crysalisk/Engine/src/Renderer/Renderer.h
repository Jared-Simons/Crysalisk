//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include "Core/Clock.h"
#include "Core/Logging.h"
#include "Defines.h"
#include "Renderer/Camera.h"
#include "Resources/ResourceTypes.h"

// Forward Declarations
class IRendererBackend;
class Platform::IPlatformWindow;
struct RenderPacket_t;

//-------------------------------------------------------------------------
// The renderer frontend.
//-------------------------------------------------------------------------
class Renderer
{
public:
	static void		AllocateRenderSystemState(Renderer** ppSystemState);

	// Initialize, shutdown.
	bool			Initialize(RendererConfig_t& pAppConfig);
	void			Shutdown();
	bool			Update(struct RenderPacket_t& pRenderData, class CMap& pCurrentMap);

	bool			RegisterWindow(Platform::IPlatformWindow& pWindow);
	void			ReleaseWindowResources(Platform::IPlatformWindow& pWindow);
	
	void			ConfigureRenderViews(Utl::CVector<RendererViewConfig_t>& pRenderViewConfigs);

	bool			AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData);
	void			ReleaseTextureResources(struct Texture* pTexture);

	bool			CreateShader(class Shader* pShader, struct ShaderConfig_t& pAppConfig);
	void			DestroyShader(class Shader* pShader);

	void			AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName);
	void			ReleaseShaderInstanceResources(class Shader* pShader, unsigned int pInstanceID);

	void			BindShader(class Shader* pShader);
	void			BindShaderPerFrameUniforms(class Shader* pShader);
	void			BindShaderPerInstanceUniforms(class Shader* pShader, unsigned int hInstanceID);

	void			SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData);

	bool			CreateRendererView(RendererViewConfig_t& pViewConfig, RendererView& pOutView);
	void			DestroyRendererView(RendererView& pView);

	bool			BeginRendererView(RendererView& pView);
	void			EndRendererView();

	bool			CreateRenderBuffer(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer);
	void			DestroyRenderBuffer(RenderBuffer& pBuffer);

	void			AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance);
	void			AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance);
	void			AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance);
	void			ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry);

	void			DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer);

	void			UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig);
	void			UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig);
	void			UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance);

	const Camera_t&	GetMainCamera() const { return m_Camera; }

	// Temp geometry stuff
	bool	CreateTempTextObjects();
	void	DestroyTempGeometry();

	void	SetMinimized(bool bMinimized) { m_bMinimized = bMinimized; }
	void	SetWindowDimensions(unsigned int iWidth, unsigned int iHeight) { m_iWindowWidth = iWidth; m_iWindowHeight = iHeight; }

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

public:
	Camera_t			m_Camera{};

	Utl::CVector<RendererView>	m_RenderViews{};

	// TODO: Remove or refactor these eventually.
	RendererView*		m_SceneView{};
	RendererView*		m_UIView{};

	RenderBuffer		m_WorldVertexBuffer{};
	RenderBuffer		m_WorldIndexBuffer{};

	RenderBuffer		m_UIVertexBuffer{};
	RenderBuffer		m_UIIndexBuffer{};

	size_t				m_iCurrentRendererFrame{ kInvalidID_U64 };
	uint8_t				m_iRendererDebugMode{ 0 };

	// A pointer to the window we're rendering to.
	Platform::IPlatformWindow*	m_pMainWindow;
};
