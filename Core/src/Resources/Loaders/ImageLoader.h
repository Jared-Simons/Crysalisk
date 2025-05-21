//=============================================================================
// Author: Jared Simons
// Date Created: 5/18/2025
// 
// Purpose: Image resource loading utilities.
//=============================================================================
#pragma once

#include <Defines.h>

namespace ImageLoader
{
	CORE_API bool LoadImage(const char* pszImagePath, struct ImageResource_t& pOutResource);
	CORE_API void ReleaseImageResource(struct ImageResource_t& pResource);
	
	struct ImageResource_t
	{
		int	Width{ 0 };
		int	Height{ 0 };
		uint8_t	NumChannels{ 0 };
		bool	bIsTransparent{ false };
		byte*	pPixelData{ nullptr };

		~ImageResource_t()
		{
			// Make sure to release any dynamically-allocated resources.
			ReleaseImageResource(*this);
		}
	};
}