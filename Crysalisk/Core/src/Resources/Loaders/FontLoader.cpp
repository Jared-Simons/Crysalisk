//=============================================================================
// Author: Jared Simons
// Date Created: 6/11/2025
// 
// Purpose: Font resource loading utilities.
//=============================================================================
#include "FontLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Vendor/stb_truetype.h"

namespace FontLoader
{
	// The codepoint of the first char we include in our fonts.
	const uint32_t kCodePointOfFirstChar = 32;
	// The number of characters we include in our font atlases.
	const uint32_t kCharsToIncludeInFontAtlas = 95;

	// @Multithreading: This function is a prime candidate for threading.
	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing font asset from disk 
	//				(NOTE: This currently only supports True-Type fonts!)
	// 
	// Input:	pszFontPath - The path to the font to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadFontResource(const char* pszFontPath, FontResource_t& pOutResource)
	{
		// Make sure the file actually exists.
		if (!FileSystem::FileExists(pszFontPath))
		{
			LOG_ERROR("Font file '%s' could not be found. Font resource loading failed.", pszFontPath);
			return false;
		}

		FileHandle_t ttfFileHandle{};
		if (!FileSystem::OpenFile(pszFontPath, eFileMode::FILEMODE_READ, true, &ttfFileHandle))
		{
			return false;
		}

		size_t ttfFileSize = FileSystem::FileSize(&ttfFileHandle);
		Utl::CVector<uint8_t> ttfBytes(ttfFileSize);

		// Read the font data into our buffer.
		FileSystem::ReadWholeBinaryFile(ttfBytes.Data(), ttfFileSize, &ttfFileSize, &ttfFileHandle);

		FileSystem::CloseFile(&ttfFileHandle);

		stbtt_fontinfo fontInfo{};
		pOutResource.FontCount = stbtt_GetNumberOfFonts(ttfBytes.Data());

		// TODO: Configurable font size.
		pOutResource.FontSize = 64;

		if (!stbtt_InitFont(&fontInfo, ttfBytes.Data(), 0))
		{
			// TODO: Logging
			CAssert(0);
		}

		// Some buffers to store font data in.
		stbtt_packedchar	packedChars[kCharsToIncludeInFontAtlas];
		stbtt_aligned_quad	alignedQuads[kCharsToIncludeInFontAtlas];
		ImageLoader::ImageResource_t& pFontAtlas = pOutResource.FontAtlasResource;

		// TODO: Configurable texture atlas dimensions.
		pOutResource.FontAtlasResource.Width = pOutResource.FontAtlasResource.Height = 512;
		pOutResource.FontAtlasResource.NumChannels = 1;

		// Allocate some memory for our font atlas texture data.
		pOutResource.FontAtlasResource.pPixelData = new byte[pFontAtlas.Width * pFontAtlas.Height];

		// Piece together our font atlas texture data.
		stbtt_pack_context tffPackContext{};
		stbtt_PackBegin(&tffPackContext, pFontAtlas.pPixelData, pFontAtlas.Width, pFontAtlas.Height, 0, 1, nullptr);
		stbtt_PackFontRange(&tffPackContext, ttfBytes.Data(), 0, pOutResource.FontSize, kCodePointOfFirstChar, kCharsToIncludeInFontAtlas, packedChars);
		stbtt_PackEnd(&tffPackContext);

		pOutResource.charData.Resize(kCharsToIncludeInFontAtlas);
		pOutResource.charQuads.Resize(kCharsToIncludeInFontAtlas);

		for (int charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
		{
			float unusedX, unusedY;
			stbtt_GetPackedQuad(packedChars, pFontAtlas.Width, pFontAtlas.Height, charIdx, &unusedX, &unusedY, &alignedQuads[charIdx], 0);

			// Copy over all the char data to our own format.
			FontChar_t& pCurrChar = pOutResource.charData[charIdx];

			pCurrChar.xMin = packedChars[charIdx].x0;
			pCurrChar.xMax = packedChars[charIdx].x1;
			pCurrChar.yMin = packedChars[charIdx].y0;
			pCurrChar.yMax = packedChars[charIdx].y1;
			pCurrChar.xOffset = packedChars[charIdx].xoff;
			pCurrChar.yOffset = packedChars[charIdx].yoff;
			pCurrChar.Advance = packedChars[charIdx].xadvance;
		}

		for (int charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
		{
			FontQuadData_t& pCurrQuad = pOutResource.charQuads[charIdx];

			pCurrQuad.sMin = alignedQuads[charIdx].s0;
			pCurrQuad.tMin = alignedQuads[charIdx].t0;
			pCurrQuad.sMax = alignedQuads[charIdx].s1;
			pCurrQuad.tMax = alignedQuads[charIdx].t1;
		}

		return true;
	}
}