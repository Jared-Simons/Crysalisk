//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

#include <Containers/CVector.h>

// Forward declarations
template <typename T> class CVector;

class Texture
{
public:
	int m_Width;
	int m_Height;
	int m_ChannelCount;
	const char* m_TextureName;

	void* m_InternalData;
};

class StaticMesh
{
public:
	Texture* m_pTexture;
	const char* m_szGeometryName;

	void* m_pInternalData;
	// TODO: Move these to the renderer backend
	size_t m_VertexDataOffset;
	size_t m_VertexDataSize;

	size_t m_IndexDataOffset;
	size_t m_IndexDataSize;
};

struct RenderPacket_t
{
	CVector<StaticMesh*> geometries;
};
