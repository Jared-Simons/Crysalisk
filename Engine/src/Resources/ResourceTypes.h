//=============================================================================
// Author: Jared Simons
// Date Created: 8/29/2024
// 
// Purpose: Asset type definitions.
//=============================================================================
#pragma once

class Texture
{
public:
	int m_Width;
	int m_Height;
	int m_ChannelCount;
	const char* m_TextureName;

	void* m_InternalData;
};
