//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include "Core/Clock.h"
#include "Core/Logging.h"
#include "Defines.h"
#include "Resources/ResourceTypes.h"

// Forward Declarations
class IRendererBackend;
class Platform::IPlatformWindow;
struct RenderPacket_t;

//-------------------------------------------------------------------------
// Renderer configuration, to be set by the application.
//-------------------------------------------------------------------------
struct RendererConfig_t
{
	// The application's name.
	const char* szApplicationName;
	// An opaque pointer to the window we'll be rendering to.
	Platform::IPlatformWindow* pMainWindow;
	// A pointer to the loaded backend module.
	Platform::ISysModule* pRendererBackendDLL;

	size_t	MaxTextures; // The amount of textures that can be loaded at once. Must match the texture system config's amount.
	size_t	MaxPipelines; // The amount of graphics pipelines that can be loaded at once. Must be greater than the maximum materials.
};

//-------------------------------------------------------------------------
// The renderer frontend.
//-------------------------------------------------------------------------
class Renderer
{
public:
	bool			Initialize(RendererConfig_t& pConfig);
	void			Shutdown();
	bool			Update(struct RenderPacket_t& pRenderData);

	bool			AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData);
	void			ReleaseTextureResources(struct Texture* pTexture);

	bool			CreateShader(class Shader* pShader, struct ShaderConfig_t& pConfig);
	void			DestroyShader(class Shader* pShader);

	void			AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID);

	void			BindShader(class Shader* pShader);
	void			BindShaderPerFrameUniforms(class Shader* pShader);
	void			BindShaderPerInstanceUniforms(class Shader* pShader, unsigned int hInstanceID);

	void			SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData);

	void			AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance);

	void			AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance);
	void			ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry);

	void			DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer);

	void			UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig);
	void			UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig);

	// Temp geometry stuff
	bool	CreateTempGeometry();
	void	DestroyTempGeometry();

	void	SetMinimized(bool bMinimized) { m_bMinimized = bMinimized; }
	void	SetWindowDimensions(unsigned int iWidth, unsigned int iHeight) { m_iWindowWidth = iWidth; m_iWindowHeight = iHeight; }

private:
	IRendererBackend* m_RendererBackend{ nullptr };
	bool	m_bMinimized{ false };
	Clock	m_RendererClock{};
	double	m_flLastFrameTime{};

	RenderMetrics_t	m_Metrics{};

	// The current dimensions of the surface we are presenting to (usually a window).
	unsigned int	m_iWindowWidth{ 0 };
	unsigned int	m_iWindowHeight{ 0 };

public:
	RendererView		m_SceneView{};
	RendererView		m_UIView{};

	RenderBuffer		m_WorldVertexBuffer{};
	RenderBuffer		m_WorldIndexBuffer{};

	RenderBuffer		m_UIVertexBuffer{};
	RenderBuffer		m_UIIndexBuffer{};

	size_t				m_iCurrentRendererFrame{ kInvalidID_U64 };
	uint8_t				m_iRendererDebugMode{ 0 };

	GeometryInstance_t	m_TestCube{};
	Mesh				m_SponzaModel;
};

extern Renderer* g_pRenderer;