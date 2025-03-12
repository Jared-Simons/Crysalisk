//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include "Defines.h"
#include "Core/Logging.h"
#include "Resources/ResourceTypes.h"

// Forward Declarations
class IRendererBackend;
struct RenderPacket_t;

struct RendererConfig_t
{
	const char* szApplicationName;
	void* pHWnd;
	Platform::ISysModule* pRendererBackendDLL;

	size_t	MaxTextures; // The amount of textures that can be loaded at once. Must match the texture system config's amount.
};

class Renderer
{
public:
	bool	Initialize(RendererConfig_t& pConfig);
	void	Shutdown();
	bool	Update(struct RenderPacket_t& pRenderData);

	bool	AcquireTextureResources(class Texture& pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData);
	void	ReleaseTextureResources(class Texture& pTexture);

	bool	AcquireMaterialResources(class Material& pMaterial);

	// Temp geometry stuff
	bool	CreateTempGeometry();

	void	SetMinimized(bool bMinimized) { m_bMinimized = bMinimized; }

private:
	IRendererBackend* m_RendererBackend{ nullptr };
	bool	m_bMinimized{ false };

	// TODO: These are temporary...
	GeometryInstance_t	m_Cube1{};
	GeometryInstance_t	m_Cube2{};
	GeometryInstance_t	m_Cube3{};
};

extern Renderer* g_pRenderer;
