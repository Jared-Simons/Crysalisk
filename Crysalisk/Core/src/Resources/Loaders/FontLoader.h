//=============================================================================
// Author: Jared Simons
// Date Created: 6/11/2025
// 
// Purpose: Font resource loading utilities.
//=============================================================================
#pragma once

#include <Defines.h>

#include "Containers/CVector.h"
#include "ImageLoader.h"

namespace FontLoader
{
	// Attempts to load an existing font asset from disk (NOTE: This currently only supports True-Type fonts!)
	CORE_API bool LoadFontResource(const char* pszFontPath, u64 iFontSize, struct FontResource_t& pOutResource);

	struct FontCharData_t
	{
		u16	xMin;
		u16	xMax;
		u16	yMin;
		u16	yMax;

		f32 kerning;
		f32	xOffset;
		f32	yOffset;
		f32	flAdvance;
		
		f32	sMin;
		f32	tMin;
		f32	sMax;
		f32	tMax;
	};

	// A font resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct FontResource_t
	{
		// The number of fonts in this resource.
		u64							FontCount{ 0 };
		// The size of this font.
		f32							FontSize{ 0.0f };
		// The scale of the font, for scaling to surface resolution.
		f32							flFontScale{ 0.0f };
		// The unit height between lines.
		u32								lineHeight{ 0 };

		Utl::CVector<FontCharData_t>	charGlyphs{};

		// The font's atlas as an image resource needed for rendering.
		ImageLoader::ImageResource_t	FontAtlasResource{};

		// Destructor.
		~FontResource_t()
		{
			// We need to free this separately from the ImageResource_t destructor.
			delete[] FontAtlasResource.pPixelData;
			FontAtlasResource.pPixelData = nullptr;
		}
	};
}