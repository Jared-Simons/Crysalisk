//=============================================================================
// Author: Jared Simons
// Date Created: 4/19/2025
// 
// Purpose: The texture manager.
//=============================================================================
#include "FontSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Platform/FileSystem.h>
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h" // TODO: REMOVE THIS ASAP!

#define STB_TRUETYPE_IMPLEMENTATION
#include <Vendor/stb_truetype.h>

const uint32_t kCodePointOfFirstChar = 32;
const uint32_t kCharsToIncludeInFontAtlas = 95;

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

		int fontCount = stbtt_GetNumberOfFonts(fontBytes.Data());

		// Allocate some memory for our font data.
		byte* pFontPixelBuffer = new uint8_t[newFont->m_Atlas.Width * newFont->m_Atlas.Height];
		newFont->m_pInternalData = new TrueTypeFontInternalData_t;
		TrueTypeFontInternalData_t* pInternalData = (TrueTypeFontInternalData_t*)newFont->m_pInternalData;

		// Font pixel height
		// TODO: Configurable
		float fontSize = 64.0f;

		stbtt_packedchar packedChars[kCharsToIncludeInFontAtlas];
		stbtt_aligned_quad alignedQuads[kCharsToIncludeInFontAtlas];

		stbtt_pack_context ctx;

		stbtt_PackBegin(&ctx, pFontPixelBuffer, newFont->m_Atlas.Width, newFont->m_Atlas.Height, 0, 1, nullptr);
		stbtt_PackFontRange(&ctx, fontBytes.Data(), 0, fontSize, kCodePointOfFirstChar, kCharsToIncludeInFontAtlas, packedChars);
		stbtt_PackEnd(&ctx);

		for (int idx = 0; idx < kCharsToIncludeInFontAtlas; idx++)
		{
			float unusedX, unusedY;
			stbtt_GetPackedQuad(pInternalData->packedChar, newFont->m_Atlas.Width, newFont->m_Atlas.Height, idx, &unusedX, &unusedY, &pInternalData->alignedQuads[idx], 0);
		}

		// Acquire texture resources.
		newFont->m_hFontTexture = g_pTextureSystem->WrapTextureInternal(pFontPath, newFont->m_Atlas.Width, newFont->m_Atlas.Height, pFontPixelBuffer); // TODO: formatted texture name

		// Set up a material for this
		// TODO: We need to get a material for now because that's the ONLY way the Vulkan renderer can set up the descriptor sets.
		// This should not be the case for fonts! This will need to change asap.
		MaterialConfig_t newFontConfig{};
		newFontConfig.pszDiffuseTextureName = StringDuplicate(pFontPath);
		newFontConfig.pszMaterialName = StringDuplicate("NewFontMaterialPogchampandwhatnot");
		newFontConfig.pszShaderName = StringDuplicate("Core.UI");
		MaterialHandle_t newMaterial = g_pMaterialSystemInternal->AcquireMaterialFromConfig(newFontConfig);

		// Delete the original pixel buffer.
		delete[] pFontPixelBuffer;
	}

	return fontIdx;
}

GeometryConfig2D_t CFontSystem::AcquireConfigForText(const char* pText, FontHandle_t hFont)
{
	GeometryConfig2D_t newConfig{};

	// TODO: Make this configurable
	int order[6] = { 0, 1, 2, 0, 2, 3 };
	float pixelScale = 2.0 / 600; // 600 is the default height of the window we create.
	float size = 1.0f;
	Vector2D position = { -0.8f, 0.4f };

	// TODO: Check for invalid fonts.

	// Get the internal font data.
	TrueTypeFontInternalData_t* pInternalFontData = reinterpret_cast<TrueTypeFontInternalData_t*>(m_FontDictionary.m_FontArray[hFont].m_pInternalData);
	CAssert(pInternalFontData != nullptr);

	// Get all the specific glyph data.
	stbtt_packedchar* pPackedChars = pInternalFontData->packedChar;
	stbtt_aligned_quad* pAlignedQuads = pInternalFontData->alignedQuads;
	size_t stringLength = StringLength(pText);
	newConfig.Vertices.Resize(stringLength * 4);

	size_t currentAdvance{ 0 };

	for (int idx = 0; idx < stringLength; idx++)
	{
		stbtt_packedchar* pCurrentPackedChar = &pPackedChars[pText[idx] - kCodePointOfFirstChar];
		stbtt_aligned_quad* pCurrentAlignedQuad = &pAlignedQuads[pText[idx] - kCodePointOfFirstChar];

		Vector2D currentGlyphSize =
		{
			(pCurrentPackedChar->x1 - pCurrentPackedChar->x0) * pixelScale * size,
			(pCurrentPackedChar->y1 - pCurrentPackedChar->y0) * pixelScale * size
		};

		Vector2D currentGlyphBBBottomLeft =
		{
			position.x + (pCurrentPackedChar->xoff * pixelScale * size),
			position.y - (pCurrentPackedChar->yoff + pCurrentPackedChar->y1 - pCurrentPackedChar->y0) * pixelScale * size
		};

		// Generate the vertex data.
		Vertex2D glyphVertices[4];
		glyphVertices[0].Pos = {currentGlyphBBBottomLeft.x + currentGlyphSize.x, currentGlyphSize.y};
		glyphVertices[0].TexCoord = {pCurrentAlignedQuad->s1, pCurrentAlignedQuad->t0};

		glyphVertices[1].Pos = {currentGlyphBBBottomLeft.x, currentGlyphBBBottomLeft.y + currentGlyphSize.y};
		glyphVertices[1].TexCoord = {pCurrentAlignedQuad->s0, pCurrentAlignedQuad->t0};

		glyphVertices[2].Pos = {currentGlyphBBBottomLeft.x, currentGlyphBBBottomLeft.y};
		glyphVertices[2].TexCoord = {pCurrentAlignedQuad->s0, pCurrentAlignedQuad->t1};

		glyphVertices[3].Pos = {currentGlyphBBBottomLeft.x + currentGlyphSize.x, currentGlyphBBBottomLeft.y};
		glyphVertices[3].TexCoord = {pCurrentAlignedQuad->s1, pCurrentAlignedQuad->t1};

		for (int glyphIdx = 0; glyphIdx < 6; glyphIdx++)
		{
			Vertex2D currentVertex;
			currentVertex.Pos = glyphVertices[order[glyphIdx]].Pos;
			currentVertex.Pos.x += currentAdvance; // Push this vertex up by the current character advance.
			currentVertex.Color = Vector4D(1.0f, 1.0f, 1.0f, 1.0f);
			currentVertex.TexCoord = glyphVertices[order[glyphIdx]].TexCoord;
			newConfig.Vertices.PushBack(currentVertex);
		}

		// Push up the advance offset.
		currentAdvance += pCurrentPackedChar->xadvance * pixelScale * size;
	}

	return newConfig;
}
