//=============================================================================
// Author: Jared Simons
// Date Created: 6/11/2025
// 
// Purpose: Font resource loading utilities.
//=============================================================================
#include "FontLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"
#include "Strings/String_Utils.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Vendor/stb_truetype.h"

namespace FontLoader
{
	// The codepoint of the first char we include in our fonts.
	const u32 kCodePointOfFirstChar = 32;
	// The number of characters we include in our font atlases.
	const u32 kCharsToIncludeInFontAtlas = 95;

	// Forward Declarations
	bool	LoadSingleTrueTypeFont(const char* pszFontPath, u64 iFontSize, bool bLoadAllGlyphs, FontResource_t& pOutResource);

	// @Multithreading: This function is a prime candidate for threading.
	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing font asset from disk 
	//				(NOTE: This currently only supports True-Type fonts!)
	// 
	// Input:	pszFontPath - The path to the font to be loaded.
	//			iFontSize - The size to create this font resource with.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadFontResource(const char* pszFontPath, u64 iFontSize, FontResource_t& pOutResource) 
	{
		if (!LoadSingleTrueTypeFont(pszFontPath, iFontSize, true, pOutResource))
		{
			return false;
		}

		return true;
	}

	bool LoadFontAtlasLayoutFile(const char* pszLayoutFilePath, Utl::CVector<GlyphLayoutData_t>& pOutData)
	{
		char szFullPath[2048]{};
		if (!FileSystem::FileExists(pszLayoutFilePath))
		{
			// Try to put the texture folder path on and go again.
			Utl::StringFormat_V("Assets/Textures/%s.txt", szFullPath, pszLayoutFilePath);

			if (!FileSystem::FileExists(szFullPath))
			{
				LOG_ERROR("Failed to find image atlas layout file '%s'.", pszLayoutFilePath);
				return false;
			}

			pszLayoutFilePath = szFullPath;
		}

		FileHandle_t layoutFile{};
		if (!FileSystem::OpenFile(pszLayoutFilePath, eFileMode::FILEMODE_READ, false, &layoutFile))
		{
			return false;
		}

		char szLineBuffer[2048]{};
		u64 bytesRead{ 0 };

		while (FileSystem::ReadLine(szLineBuffer, 2048, &bytesRead, &layoutFile))
		{
			if (Utl::StringLength(szLineBuffer) == 0 || szLineBuffer[0] == '#')	// Skip blank lines and comments
			{
				continue;
			}

			u64 idx = pOutData.PushBack(GlyphLayoutData_t{});
			GlyphLayoutData_t* pGlyph = &pOutData[idx];

			char* pszTrimmedGlyphName = Utl::StringTrim(szLineBuffer);
			pGlyph->pszGlyphName = Utl::StringStrip(pszTrimmedGlyphName, '\"');

			// Process the glyph data
			char szSubLineBuffer[2048]{};
			u64 subBytesRead{ 0 };
			while (FileSystem::ReadLine(szSubLineBuffer, 2048, &subBytesRead, &layoutFile))
			{
				if (szSubLineBuffer[0] == '}')	// Closing brace
				{
					break;
				}

				u32 spaceIndex = Utl::StringIndexOf(szSubLineBuffer, ' ');
				if (spaceIndex == kInvalidID_U32)
				{
					continue;
				}

				char* pszKey = Utl::StringDuplicate_N(&szSubLineBuffer[1], spaceIndex - 1);
				char* pszTrimmedKey = Utl::StringTrim(pszKey);

				if (Utl::StringCompareInsensitive("\"MinX\"", pszTrimmedKey))
				{
					pGlyph->xMin = Utl::String_ATOI(&szSubLineBuffer[spaceIndex + 2]);	// + 2 to skip the space and first '"'.
				} 
				else if (Utl::StringCompareInsensitive("\"MaxX\"", pszTrimmedKey))
				{
					pGlyph->xMax = Utl::String_ATOI(&szSubLineBuffer[spaceIndex + 2]);	// + 2 to skip the space and first '"'.
				}
				else if (Utl::StringCompareInsensitive("\"MinY\"", pszTrimmedKey))
				{
					pGlyph->yMin = Utl::String_ATOI(&szSubLineBuffer[spaceIndex + 2]);	// + 2 to skip the space and first '"'.
				}
				else if (Utl::StringCompareInsensitive("\"MaxY\"", pszTrimmedKey))
				{
					pGlyph->yMax = Utl::String_ATOI(&szSubLineBuffer[spaceIndex + 2]);	// + 2 to skip the space and first '"'.
				}
				else
				{
					CAssert(0);
				}

				Utl::StringFree(pszTrimmedKey);
				pszTrimmedKey = nullptr;
				Utl::StringFree(pszKey);
				pszKey = nullptr;
			}

			Utl::StringFree(pszTrimmedGlyphName);
			pszTrimmedGlyphName = nullptr;
		}

		FileSystem::CloseFile(&layoutFile);
		
		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load a single true-type font from disk.
	// 
	// Input:	pszFontPath - The path of the .ttf file to be loaded.
	//			iFontSize - The size to create this font resource with.
	//			bLoadAllGlyphs - Indicates whether or not to load all glyphs associated with this font,
	//				or just specific glyphs.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
	bool LoadSingleTrueTypeFont(const char* pszFontPath, u64 iFontSize, bool bLoadAllGlyphs, FontResource_t& pOutResource)
	{
		u64 iTTFFileSize = 0;
		u64 iBytesRead = 0;
		Utl::CVector<u8> ttfBytes = {};
		u8* pBufferPtr = nullptr;

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

		pOutResource.flFontScale = stbtt_ScaleForPixelHeight(&fontInfo, (f32)iFontSize);

		i32 ascent, descent, lineGap;
		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
		pOutResource.lineHeight = (u32)((ascent - descent + lineGap) * pOutResource.flFontScale);

		if (bLoadAllGlyphs)
		{
			pOutResource.charGlyphs.Resize(kCharsToIncludeInFontAtlas);

			// Setup our atlas resource.
			ImageLoader::ImageResource_t& pImageResource = pOutResource.FontAtlasResource;
			pImageResource.Width = 512;
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
				(f32)iFontSize,
				kCodePointOfFirstChar,
				kCharsToIncludeInFontAtlas,
				packedChars);

			stbtt_PackEnd(&packContext);

			// Make sure to get our quad texcoords as well.
			for (i32 charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
			{
				f32 flUnusedX, flUnusedY;
				stbtt_GetPackedQuad(
					packedChars,
					pImageResource.Width,
					pImageResource.Height,
					charIdx,
					&flUnusedX, &flUnusedY,
					&alignedQuads[charIdx], 0);
			}

			// Get kerning data.
			i32 kerningCount = stbtt_GetKerningTableLength(&fontInfo);

			// Save out all of our glyph data to the resource.
			for (i32 charIdx = 0; charIdx < kCharsToIncludeInFontAtlas; charIdx++)
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
}