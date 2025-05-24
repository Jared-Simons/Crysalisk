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
#include <Resources/Loaders/ShaderLoader.h> // TODO: Move types to their own files!

// Forward Declarations
class IRendererBackend;
struct RenderPacket_t;

struct RendererConfig_t
{
	const char* szApplicationName;
	void* pHWnd;
	Platform::ISysModule* pRendererBackendDLL;

	size_t	MaxTextures; // The amount of textures that can be loaded at once. Must match the texture system config's amount.
	size_t	MaxPipelines; // The amount of graphics pipelines that can be loaded at once. Must be greater than the maximum materials.
};

class Renderer
{
public:
	bool	Initialize(RendererConfig_t& pConfig);
	void	Shutdown();
	bool	Update(struct RenderPacket_t& pRenderData);

	bool			AcquireTextureResources(class Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData);
	void			ReleaseTextureResources(class Texture* pTexture);

	bool	AcquireTextureResources_OLD(class Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData);
	void	ReleaseTextureResources_OLD(class Texture& pTexture);

	bool	AcquireMaterialResources(class Material& pMaterial, struct MaterialInstanceData_t& pMaterialInstanceData);

	bool			CreateShader(class Shader* pShader, struct ShaderConfig_t& pConfig);
	void			DestroyShader(class Shader* pShader);

	void			BindShader(class Shader* pShader);
	void			BindShaderPerFrameUniforms(class Shader* pShader);

	bool			AcquireShaderResources_OLD(class Shader& pShader, struct ShaderConfig_t& pShaderConfig);
	void			ReleaseShaderResources(class Shader& pShader);

	void			BindShader_OLD(class Shader& pShader);

	void			AcquireGeometryResources(GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance);

	void			AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance);
	void			ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry);

	void			DrawGeometry(GeometryInstance_t& pGeometry);

	// Temp geometry stuff
	bool	CreateTempGeometry();
	void	DestroyTempGeometry();

	void	SetMinimized(bool bMinimized) { m_bMinimized = bMinimized; }

private:
	IRendererBackend* m_RendererBackend{ nullptr };
	bool	m_bMinimized{ false };
	Clock	m_RendererClock{};
	double	m_flLastFrameTime{};

	// TODO: These are temporary...
public:
	RendererView		m_SceneView{};
	RendererView		m_UIView{};

	RenderBuffer		m_WorldVertexBuffer{};
	RenderBuffer		m_WorldIndexBuffer{};

	RenderBuffer		m_UIVertexBuffer{};
	RenderBuffer		m_UIIndexBuffer{};

	GeometryInstance_t	m_Cube1{};
	GeometryInstance_t	m_Cube2{};
	GeometryInstance_t	m_Cube3{};
	Mesh	m_SponzaModel;
};

extern Renderer* g_pRenderer;
