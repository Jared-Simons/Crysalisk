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
	struct ImageLoaderParams_t
	{
		const char*				pszImageName{ nullptr };
		struct ImageResource_t* pOutResource{ nullptr };
	};

	// Attempts to load an existing image asset from disk.
	CORE_API bool LoadImage(const char* pszImagePath, struct ImageResource_t& pOutResource);

	CORE_API i32 LoadImage_Threaded(void* pParams, void** pJobResults);
	
	// Releases resources allocated during image resource acquisition.
	CORE_API void ReleaseImageResource(struct ImageResource_t& pResource);
	
	// An image resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct ImageResource_t
	{
		i32		Width{ 0 };
		i32		Height{ 0 };
		u8	NumChannels{ 0 };
		bool	bIsTransparent{ false };
		byte*	pPixelData{ nullptr };

		~ImageResource_t()
		{
			// Make sure to release any dynamically-allocated resources.
			ReleaseImageResource(*this);
		}
	};
}