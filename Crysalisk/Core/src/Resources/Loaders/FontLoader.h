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
	CORE_API bool LoadFontResource(const char* pszFontPath, struct FontResource_t& pOutResource);

	struct FontCharData_t
	{
		unsigned short	xMin;
		unsigned short	xMax;
		unsigned short	yMin;
		unsigned short	yMax;
		float			xOffset;
		float			yOffset;
		float			flAdvance;
		
		float			sMin;
		float			tMin;
		float			sMax;
		float			tMax;
	};

	CORE_API bool LoadFontResource_OLD(const char* pszFontPath, struct FontResource_t& pOutResource);

	// Contains specific render data for a font character.
	struct FontChar_t
	{
		unsigned short xMin;
		unsigned short xMax;
		unsigned short yMin;
		unsigned short yMax;
		float xOffset;
		float yOffset;
		float Advance;
	};

	// TODO: Can this be combined into the struct above?
	struct FontQuadData_t
	{
		float sMin;
		float tMin;
		float sMax;
		float tMax;
	};

	// A font resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct FontResource_t
	{
		// The number of fonts in this resource.
		size_t							FontCount{ 0 };
		// The size of this font.
		float							FontSize{ 0.0f };
		// The scale of the font, for scaling to surface resolution.
		float							flFontScale{ 0.0f };

		Utl::CVector<FontCharData_t>	charGlyphs{};

		// TODO: DEPRECATE THESE!
		// An array of data needed for rendering these characters.
		Utl::CVector<FontChar_t>		charData{};
		Utl::CVector<FontQuadData_t>	charQuads{};

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