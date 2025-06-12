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
	CORE_API bool LoadFontResource(const char* pszFontPath, struct FontResource_t& pOutResource);

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

	struct FontQuadData_t
	{
		float sMin;
		float tMin;
		float sMax;
		float tMax;
	};

	struct FontResource_t
	{
		size_t							FontCount{ 0 };
		unsigned int					FontSize{ 0 };

		Utl::CVector<FontChar_t>		charData{};
		Utl::CVector<FontQuadData_t>	charQuads{};

		ImageLoader::ImageResource_t	FontAtlasResource{};

		~FontResource_t()
		{
			// We need to free this separately from the ImageResource_t destructor.
			delete[] FontAtlasResource.pPixelData;
			FontAtlasResource.pPixelData = nullptr;
		}
	};
}