//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include <Engine/IRendererBackend.h>

class CVulkanRenderer : public IRendererBackend
{
public:
	bool	RendererBackend_Initialize() override { return true; }
	void	RendererBackend_Shutdown() override {}
	bool	RendererBackend_Update(struct RenderPacket_t* pRenderData) override { return true; }

	bool	CreateTextureResources(Texture& pTexture, byte* pPixelData) override { return true; }
	void	DestroyTextureResources(Texture& pTexture) override {}

	bool	CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices) override { return true; }
	void	DestroyGeometryResources(StaticMesh& pMesh) override {}
};
