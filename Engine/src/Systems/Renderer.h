//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include "Defines.h"

// Forward Declarations
class IRendererBackend;

class LinearAllocator;
class Texture;
class StaticMesh;
class Vertex3D;
struct RenderPacket_t;

template <typename T> class CVector;

class Renderer
{
public:
	static bool		Renderer_Create(Renderer* statePtr, LinearAllocator* pAllocator);
	static void		Renderer_Shutdown(Renderer* statePtr, bool bOwnsMemory = false);

	bool	Initialize();
	void	Shutdown();
	bool	Update(struct RenderPacket_t* pRenderData);

	bool	CreateTextureResources(Texture& pTexture, byte* pPixelData);
	void	DestroyTextureResources(Texture& pTexture);

	bool	CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices, const char* szTextureName, const char* szGeometryName);
	void	DestroyGeometryResources(StaticMesh& pMesh);

private:
	IRendererBackend* m_RendererBackend{ nullptr };
};
