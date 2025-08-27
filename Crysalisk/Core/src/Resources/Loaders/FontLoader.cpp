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

	// Forward Declarations
	bool	LoadSingleTrueTypeFont(const char* pszFontPath, bool bLoadAllGlyphs, FontResource_t& pOutResource);

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
		if (!LoadSingleTrueTypeFont(pszFontPath, true, pOutResource))
		{
			return false;
		}

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation
	// 
	// Input:	
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadSingleTrueTypeFont(const char* pszFontPath, bool bLoadAllGlyphs, FontResource_t& pOutResource)
	{
		size_t iTTFFileSize = 0;
		size_t iBytesRead = 0;
		Utl::CVector<uint8_t> ttfBytes = {};
		uint8_t* pBufferPtr = nullptr;

		stbtt_fontinfo	fontInfo = {};
		// These will hold our intermediate font data.
		stbtt_packedchar	packedChars[kCharsToIncludeInFontAtlas] = {};
		stbtt_aligned_quad	alignedQuads[kCharsToIncludeInFontAtlas] = {};

		// Ensure the file actually exists.
		if (!FileSystem::FileExists(pszFontPath))
		{
			LOG_ERROR("Font file '%s' could not be found. Found resource loading failed.", pszFontPath);
			return false;
		}

		FileHandle_t hTTFFile{};
		if (!FileSystem::OpenFile(pszFontPath, eFileMode::FILEMODE_READ, true, &hTTFFile))
		{
			return false;
		}

		iTTFFileSize = FileSystem::FileSize(&hTTFFile);
		ttfBytes.Resize(iTTFFileSize);

		// Read the whole file into our buffer.
		FileSystem::ReadWholeBinaryFile(ttfBytes.Data(), iTTFFileSize, &iBytesRead, &hTTFFile);
		pBufferPtr = ttfBytes.Data();

		// Parse the font data.
		stbtt_InitFont(&fontInfo, ttfBytes.Data(), 0);	// No offset for single-font files.

		pOutResource.flFontScale = stbtt_ScaleForPixelHeight(&fontInfo, 12);	// TODO: Take in a font size as a parameter!

		if (bLoadAllGlyphs)
		{
			pOutResource.charGlyphs.Resize(kCharsToIncludeInFontAtlas);

			// Setup our atlas resource.
			ImageLoader::ImageResource_t& pImageResource = pOutResource.FontAtlasResource;
			pImageResource.Width = 512;	// TODO: Configurable?
			pImageResource.Height = 512;
			pImageResource.NumChannels = 1; 

			// Make sure to allocate space for the pixel data.
			pImageResource.pPixelData = new byte[pImageResource.Width * pImageResource.Height * pImageResource.NumChannels];

			// Pack all our font data into an atlas used for rendering.
			stbtt_pack_context	packContext{};

			stbtt_PackBegin(
				&packContext,
				pImageResource.pPixelData,
				pImageResource.Width,
				pImageResource.Height,
				0,
				1,
				nullptr);

			stbtt_PackFontRange(
				&packContext,
				ttfBytes.Data(),
				0,
				20, // TODO: Configurable font size
				kCodePointOfFirstChar,
				kCharsToIncludeInFontAtlas,
				packedChars);

			stbtt_PackEnd(&packContext);

			// Make sure to get our quad texcoords as well.
			for (int charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
			{
				float flUnusedX, flUnusedY;
				stbtt_GetPackedQuad(
					packedChars,
					pImageResource.Width,
					pImageResource.Height,
					charIdx,
					&flUnusedX, &flUnusedY,
					&alignedQuads[charIdx], 0);
			}

			// Save out all of our glyph data to the resource.
			for (int charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
			{
				FontCharData_t&		pCurrGlyph = pOutResource.charGlyphs[charIdx];
				stbtt_packedchar&	pCurrChar = packedChars[charIdx];
				stbtt_aligned_quad& pCurrQuad = alignedQuads[charIdx];

				// Geometry data.
				pCurrGlyph.xMin = pCurrChar.x0;
				pCurrGlyph.xMax = pCurrChar.x1;
				pCurrGlyph.yMin = pCurrChar.y0;
				pCurrGlyph.yMax = pCurrChar.y1;
				pCurrGlyph.xOffset = pCurrChar.xoff;
				pCurrGlyph.yOffset = pCurrChar.yoff;
				pCurrGlyph.flAdvance = pCurrChar.xadvance;

				// Texcoord data.
				pCurrGlyph.sMin = pCurrQuad.s0;
				pCurrGlyph.sMax = pCurrQuad.s1;
				pCurrGlyph.tMin = pCurrQuad.t0;
				pCurrGlyph.tMax = pCurrQuad.t1;
			}

			// Set our count for later.
			pOutResource.charGlyphs.SetCount(kCharsToIncludeInFontAtlas);
		}
		else
		{
			// LEFTOFF: Implement this!
		}

		FileSystem::CloseFile(&hTTFFile);

		return true;
	}

	bool LoadFontResource_OLD(const char* pszFontPath, FontResource_t& pOutResource)
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
		pOutResource.FontSize = 12;

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