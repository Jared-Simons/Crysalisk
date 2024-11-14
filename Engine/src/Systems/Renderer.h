//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include "Defines.h"
#include "Core/Logging.h"
#include <Math/MatrixTypes.h>
#include <Memory/LinearAllocator.h>

// Forward Declarations
class IRendererBackend;

class LinearAllocator;
class Texture;
class StaticMesh;
class Vertex3D;
struct RenderPacket_t;

template <typename T> class CVector;

// TODO: Move this definition to the .cpp file
class Renderer
{
public:
	void* operator new(u64 size, LinearAllocator* pAllocator)
	{
		void* pBlock = pAllocator->Allocate(size);
		if (pBlock)
		{
			return pBlock;
		}
		else
		{
			LOG_FATAL("Systems allocator failed to allocator memory for the renderer!");
			return false;
		}
	}

	// NOTE: This does nothing and just exists to clear a compiler warning, 
	// the memory was allocated and will be freed when the linear allocator gets cleared.
	// If the creation of this system fails, the engine will shutdown regardless.
	void	operator delete(void* pMemory, LinearAllocator* pAllocator) {}

	static Renderer*		Renderer_Create(LinearAllocator* pAllocator);

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

extern Renderer* gpRenderer;
