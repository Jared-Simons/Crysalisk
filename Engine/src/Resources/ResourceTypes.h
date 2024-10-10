//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

#include "Defines.h"

class Texture
{
public:
	u64 m_Width;
	u64 m_Height;
	u64 m_ChannelCount;
	const char* m_TextureName;

	void* m_internalData;
};

class StaticMesh
{
public:
	Texture* m_pTexture;
	const char* m_szGeometryName;

	void* m_pinternalData;
};

struct RenderPacket_t
{
	StaticMesh* geometries[10];
};
