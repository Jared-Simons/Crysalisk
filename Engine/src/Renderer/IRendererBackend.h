//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The interface to the renderer backend
// Not to be accessed directly! Rather through the renderer frontend
//=============================================================================
#pragma once

#include "Defines.h"

// Forward declarations
class Texture;
class StaticMesh;
class Vertex3D;

template <typename T> class CVector;

class IRendererBackend
{
public:
	virtual bool	RendererBackend_Initialize() = 0;
	virtual void	RendererBackend_Shutdown() = 0;
	virtual bool	RendererBackend_Update(struct RenderPacket_t* pRenderData) = 0;

	virtual bool	CreateTextureResources(Texture& pTexture, byte* pPixelData) = 0;
	virtual void	DestroyTextureResources(Texture& pTexture) = 0;

	virtual bool	CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices) = 0;
	virtual void	DestroyGeometryResources(StaticMesh& pMesh) = 0;
};

