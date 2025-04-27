//=============================================================================
// Author: Jared Simons
// Date Created: 4/19/2025
// 
// Purpose: The font manager.
//=============================================================================
#include "FontSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h" // TODO: REMOVE THIS ASAP!

#define STB_TRUETYPE_IMPLEMENTATION
#include <Vendor/stb_truetype.h>

const uint32_t kCodePointOfFirstChar = 32;
const uint32_t kCharsToIncludeInFontAtlas = 95;

void TextObject::SetText(const char* pszText)
{
	if (m_pszText)
		StringFree(m_pszText);
	m_pszText = StringDuplicate(pszText);

	// Make sure to release the old renderer resources.
	if (m_Geometry.vertexDataOffset != kInvalidID_U64)
		g_pRenderer->ReleaseGeometryResources(g_pRenderer->m_UIVertexBuffer, m_Geometry);

	// Get new geometry for our text.
	GeometryConfig2D_t newConfig = g_pFontSystem->AcquireConfigForText(m_pszText, m_hFont);
	g_pRenderer->AcquireGeometryResources(g_pRenderer->m_UIVertexBuffer, newConfig, m_Geometry);
}

//-------------------------------------------------------------------------
// Font System Implementation.
//-------------------------------------------------------------------------
class CFontSystem : public IFontSystem
{
	virtual bool	Initialize(FontSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual FontHandle_t	AcquireFontResources(const char* pFontPath) override;

	virtual GeometryConfig2D_t	AcquireConfigForText(const char* pText, FontHandle_t hFont) override;

private:
	FontSystemConfig_t m_Config{};

	struct FontLookup
	{
		FontHandle_t	FontIndex{ kInvalidID_U32 };
	};

	struct FontDictionary
	{
		Utl::CHashMap<const char*, FontLookup> m_FontHashmap{};
		Font* m_FontArray{ nullptr };
	};

	FontDictionary	m_FontDictionary{};
};

// Font System context.
static CFontSystem s_FontSystem;
IFontSystem* g_pFontSystem = &s_FontSystem;

struct TrueTypeFontInternalData_t
{
	stbtt_packedchar packedChar[kCharsToIncludeInFontAtlas];
	stbtt_aligned_quad alignedQuads[kCharsToIncludeInFontAtlas];
};

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CFontSystem::Initialize(FontSystemConfig_t config)
{
	m_Config = config;
	m_FontDictionary.m_FontHashmap.Resize(config.MaxFonts);

	// Allocate memory for our font array
	m_FontDictionary.m_FontArray = (Font*)new Font[config.MaxFonts];

	LOG_INFO("Font system initialized! (Max fonts: %d)", config.MaxFonts);
	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CFontSystem::Shutdown()
{
	delete[] m_FontDictionary.m_FontArray;
}

//-------------------------------------------------------------------------
// Loads a font at the provided path if it hasn't been loaded.
//-------------------------------------------------------------------------
FontHandle_t CFontSystem::AcquireFontResources(const char* pFontPath)
{
	Font* newFont = nullptr;
	int fontIdx = 0;

	// Make sure our state is ready to go.
	if (!m_FontDictionary.m_FontArray)
	{
		LOG_WARN("CFontSystem::AcquireFontResources() called before font system was initialized! Returning NULL handle.");
		return 0;
	}

	if (!pFontPath)
	{
		LOG_ERROR("CFontSystem::AcquireFontResources() requires a path to the font to load! Returning NULL handle.");
		return 0;
	}

	// Get an entry into the hashmap.
	FontLookup* pNewFontLookup = m_FontDictionary.m_FontHashmap.Insert(pFontPath);

	if (!pNewFontLookup)
	{
		LOG_ERROR("Failed to find free space for a new font in font hashmap! Returning NULL handle.");
		return 0;
	}

	if (pNewFontLookup->FontIndex == kInvalidID_U32) // This font does not exist yet.
	{
		// Get the first free font slot.
		for (fontIdx = 0; fontIdx < m_Config.MaxFonts; fontIdx++)
		{
			if (!m_FontDictionary.m_FontArray[fontIdx].m_pInternalData)
			{
				// Found one!
				newFont = &m_FontDictionary.m_FontArray[fontIdx];
				break;
			}
		}

		if (!newFont)
		{
			LOG_ERROR("Could not load font '%s' because all font slots are full, returning default font.", pFontPath);
			return 0; // TODO: return default?
		}

		// TODO: make this configurable.
		newFont->m_Atlas.Width = newFont->m_Atlas.Height = 512;

		// Read in the data from the file.
		Utl::CVector<uint8_t> fontBytes;
		FileHandle_t ttfFileHandle{};
		size_t ttfFileSize{ 0 };
		if (!FileSystem::OpenFile(pFontPath, eFileMode::FILEMODE_READ, true, &ttfFileHandle))
		{
			LOG_ERROR("Failed to find '%s' font file", pFontPath);
			return 0; // TODO: Default?
		}

		// Read the data into our buffer.
		ttfFileSize = FileSystem::FileSize(&ttfFileHandle);
		fontBytes.Resize(ttfFileSize);
		FileSystem::ReadWholeBinaryFile(fontBytes.Data(), ttfFileSize, &ttfFileSize, &ttfFileHandle);

		FileSystem::CloseFile(&ttfFileHandle);

		stbtt_fontinfo fontInfo = {};
		int fontCount = stbtt_GetNumberOfFonts(fontBytes.Data());

		if (!stbtt_InitFont(&fontInfo, fontBytes.Data(), 0))
		{
			// TODO: Logging
			CAssert(0);
		}

		// Allocate some memory for our font data.
		byte* pFontPixelBuffer = new uint8_t[newFont->m_Atlas.Width * newFont->m_Atlas.Height];
		newFont->m_pInternalData = new TrueTypeFontInternalData_t;
		TrueTypeFontInternalData_t* pInternalData = (TrueTypeFontInternalData_t*)newFont->m_pInternalData;

		// Font pixel height
		// TODO: Configurable
		float fontSize = 64.0f;

		stbtt_packedchar* packedChars = pInternalData->packedChar;
		stbtt_aligned_quad* alignedQuads = pInternalData->alignedQuads;

		stbtt_pack_context ctx;

		stbtt_PackBegin(&ctx, pFontPixelBuffer, newFont->m_Atlas.Width, newFont->m_Atlas.Height, 0, 1, nullptr);
		stbtt_PackFontRange(&ctx, fontBytes.Data(), 0, fontSize, kCodePointOfFirstChar, kCharsToIncludeInFontAtlas, packedChars);
		stbtt_PackEnd(&ctx);

		for (int idx = 0; idx < kCharsToIncludeInFontAtlas; idx++)
		{
			float unusedX, unusedY;
			//CAssert(kCodePointOfFirstChar + idx != 'A');
			stbtt_GetPackedQuad(pInternalData->packedChar, newFont->m_Atlas.Width, newFont->m_Atlas.Height, idx, &unusedX, &unusedY, &pInternalData->alignedQuads[idx], 0);
		}

		// Acquire texture resources.
		newFont->m_hFontTexture = g_pTextureSystem->WrapTextureInternal(pFontPath, newFont->m_Atlas.Width, newFont->m_Atlas.Height, pFontPixelBuffer); // TODO: formatted texture name

		// Set up a material for this
		// TODO: We need to get a material for now because that's the ONLY way the Vulkan renderer can set up the descriptor sets.
		// This should not be the case for fonts! This will need to change asap.
		MaterialConfig_t newFontConfig{};
		newFontConfig.pszDiffuseTextureName = StringDuplicate(pFontPath);
		newFontConfig.pszMaterialName = StringDuplicate("NewFontMaterial");
		newFontConfig.pszShaderName = StringDuplicate("Core.UI");
		MaterialHandle_t newMaterial = g_pMaterialSystemInternal->AcquireMaterialFromConfig(newFontConfig);

		// Delete the original pixel buffer.
		delete[] pFontPixelBuffer;
	}

	return fontIdx;
}

// Shoutouts to shreyaspranav for the stb_truetype example! :)
// https://github.com/shreyaspranav/stb-truetype-example/blob/main/Main.cpp
GeometryConfig2D_t CFontSystem::AcquireConfigForText(const char* pText, FontHandle_t hFont)
{
	// TODO: Handle invalid fonts
	GeometryConfig2D_t newConfig{};
	newConfig.Vertices.Resize(1024); // TODO: Resize is still broken and causing issues with our allocator. We need to fix this.

	TrueTypeFontInternalData_t* pInternalData = reinterpret_cast<TrueTypeFontInternalData_t*>(m_FontDictionary.m_FontArray[hFont].m_pInternalData);
	size_t currVertIndex{ 0 };

	// TODO: Configurable position and size
	Vector2D position{ -0.9f, -0.9f };
	float flSize{ 0.75f };
	float flPixelScale = 2.0f / 600.0f; // 600 is the default window height. TODO: CONFIGURABLE!
	
	int vertOrder[6] = { 0, 1, 2, 0, 2, 3 };
	
	size_t stringLength = StringLength(pText);

	for (int ch = 0; ch < stringLength; ch++)
	{
		//CAssert(pText[ch] != '!');
		// Make sure the character is in the font atlas.
		if (pText[ch] < kCodePointOfFirstChar || pText[ch] > kCharsToIncludeInFontAtlas + kCharsToIncludeInFontAtlas)
		{
			CAssert(0);
			continue;
		}

		// Get the data that is used in the rendering of this character.
		stbtt_packedchar* pCurrPackedChar = &pInternalData->packedChar[pText[ch] - kCodePointOfFirstChar];
		stbtt_aligned_quad* pCurrAlignedQuad = &pInternalData->alignedQuads[pText[ch] - kCodePointOfFirstChar];

		// Scale the units above from pixels by flPixelScale.
		Vector2D vecGlyphSize =
		{
			(pCurrPackedChar->x1 - pCurrPackedChar->x0) * flPixelScale * flSize,
			(pCurrPackedChar->y1 - pCurrPackedChar->y0) * flPixelScale * flSize
		};

		Vector2D vecGlyphBoundingBoxBottomLeft =
		{
			position.x + (pCurrPackedChar->xoff * flPixelScale * flSize),
			position.y - (pCurrPackedChar->yoff + pCurrPackedChar->y1 - pCurrPackedChar->y0) * flPixelScale * flSize
		};

		// The order of vertices of a quad go from top-right, top-left, bottom-left, bottom-right.
		Vector2D vecGlyphVertices[4] =
		{
			{ vecGlyphBoundingBoxBottomLeft.x + vecGlyphSize.x, vecGlyphBoundingBoxBottomLeft.y + vecGlyphSize.y },
			{ vecGlyphBoundingBoxBottomLeft.x, vecGlyphBoundingBoxBottomLeft.y + vecGlyphSize.y },
			{ vecGlyphBoundingBoxBottomLeft.x, vecGlyphBoundingBoxBottomLeft.y },
			{ vecGlyphBoundingBoxBottomLeft.x + vecGlyphSize.x, vecGlyphBoundingBoxBottomLeft.y }
		};

		Vector2D vecGlyphTexCoords[4] =
		{
			{ pCurrAlignedQuad->s1, pCurrAlignedQuad->t0 },
			{ pCurrAlignedQuad->s0, pCurrAlignedQuad->t0 },
			{ pCurrAlignedQuad->s0, pCurrAlignedQuad->t1 },
			{ pCurrAlignedQuad->s1, pCurrAlignedQuad->t1 }
		};

		// Fill up our vertex config based on the 'orders' array above.
		// TODO: Index buffers
		for (int idx = 0; idx < 6; idx++)
		{
			//Vertex2D currVertex;
			newConfig.Vertices[currVertIndex + idx].Pos = vecGlyphVertices[vertOrder[idx]];
			newConfig.Vertices[currVertIndex + idx].Color = Vector4D(1.0f, 1.0f, 1.0f, 1.0f);
			newConfig.Vertices[currVertIndex + idx].TexCoord = vecGlyphTexCoords[vertOrder[idx]];
			//currVertex.Pos = vecGlyphVertices[vertOrder[idx]];
			//currVertex.Color = Vector4D(1.0f, 1.0f, 1.0f, 1.0f);
			//currVertex.TexCoord = vecGlyphTexCoords[vertOrder[idx]];
			//newConfig.Vertices.PushBack(currVertex);
		}

		currVertIndex += 6;

		// Push the current position forward by the current character's advance.
		position.x += pCurrPackedChar->xadvance * flPixelScale * flSize;
	}
	newConfig.Vertices.SetCount(currVertIndex);

	return newConfig;
}
