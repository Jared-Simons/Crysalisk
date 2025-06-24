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
#include <Resources/Loaders/FontLoader.h>
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h" // TODO: REMOVE THIS ASAP!
#include "Systems/ShaderSystem.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <Vendor/stb_truetype.h>

//-------------------------------------------------------------------------
// Indices for each of the UI shader lookups.
//-------------------------------------------------------------------------
// UI Shader Locations
struct UIShaderLocations_t
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	texture_maps{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
};

static UIShaderLocations_t s_UIShaderLocations{};

const uint32_t kCodePointOfFirstChar = 32;
const uint32_t kCharsToIncludeInFontAtlas = 95;

void TextObject::SetText(const char* pszText)
{
	if (m_pszText)
		StringFree(m_pszText);
	m_pszText = StringDuplicate(pszText);

	// Make sure to release the old renderer resources.
	if (m_Geometry.vertexDataOffset != kInvalidID_U64)
		g_pRenderer->ReleaseGeometryResources(g_pRenderer->m_UIVertexBuffer, g_pRenderer->m_UIIndexBuffer, m_Geometry);

	// Get new geometry for our text.
	GeometryConfig2D_t newConfig = g_pFontSystem->AcquireConfigForText(m_pszText, m_hFont);
	g_pRenderer->AcquireGeometryResources(g_pRenderer->m_UIVertexBuffer, g_pRenderer->m_UIIndexBuffer, newConfig, m_Geometry);
}

//-------------------------------------------------------------------------
// Font System Implementation.
//-------------------------------------------------------------------------
class CFontSystem : public IFontSystem
{
	virtual bool	Initialize(FontSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual FontHandle_t		AcquireFontResources(const char* pFontName) override;

	virtual FontHandle_t		AcquireFontResources_OLD(const char* pFontPath) override;

	virtual GeometryConfig2D_t	AcquireConfigForText(const char* pText, FontHandle_t hFont) override;

private:
	FontHandle_t	GetFirstFreeFontHandle() const;

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
	ShaderHandle_t	m_hDefaultUIShader{ kInvalidID_U32 };
};

// Font System context.
static CFontSystem s_FontSystem;
IFontSystem* g_pFontSystem = &s_FontSystem;

struct TrueTypeFontInternalData_t
{
	stbtt_packedchar packedChar[kCharsToIncludeInFontAtlas];
	stbtt_aligned_quad alignedQuads[kCharsToIncludeInFontAtlas];
};

struct InternalFontAtlasData_t
{
	Utl::CVector<FontLoader::FontChar_t>		charData{};
	Utl::CVector<FontLoader::FontQuadData_t>	quadData{};
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

	// Set up our required shader resources.
	m_hDefaultUIShader = g_pShaderSystem->AcquireShader("Core.UI");
	
	if (m_hDefaultUIShader == kInvalidID_U32)
	{
		LOG_ERROR("Failed to acquire necessary shader resources for font rendering!");
		return false;
	}

	// Get all our shader uniform locations
	s_UIShaderLocations.projection = g_pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "projection");
	s_UIShaderLocations.view = g_pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "view");
	s_UIShaderLocations.color = g_pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "color");
	s_UIShaderLocations.texture_maps = g_pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "texture_maps");
	s_UIShaderLocations.model = g_pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "model");

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
// TODO: Documentation.
//-------------------------------------------------------------------------
FontHandle_t CFontSystem::AcquireFontResources(const char* pszFontName)
{
	FontLoader::FontResource_t	fontResource{};
	FontHandle_t				fontHandle{ kInvalidID_U32 };
	FontLookup*					pFontLookup{ nullptr };
	Font*						pFont{ nullptr };

	if (!pszFontName)
	{
		LOG_ERROR("AcquireFontResources() was called with null font name! Returning default font.");
		return 0; // TODO: Default font.
	}

	// Check to see if the font is already loaded.
	pFontLookup = &m_FontDictionary.m_FontHashmap[pszFontName];

	if (pFontLookup->FontIndex == kInvalidID_U32)
	{
		// TODO: Don't hard-code this path.
		// The font is not yet loaded, load it up.
		if (!FontLoader::LoadFontResource(pszFontName, fontResource))
		{
			LOG_ERROR("Failed to load font resource data for font '%s'. Returning default font.", pszFontName);
			return 0; // TODO: Default font.
		}

		fontHandle = GetFirstFreeFontHandle();
		pFont = &m_FontDictionary.m_FontArray[fontHandle];

		// Load up the texture data.
		TextureHandle_t hFontAtlas = g_pTextureSystem_Internal->AcquireTextureFromResource(fontResource.FontAtlasResource, pszFontName, true);

		// Get our shader instance data and bind the font atlas.
		g_pShaderSystem->AcquireShaderInstanceResources(m_hDefaultUIShader, &pFont->m_InternalInstanceID);
		g_pShaderSystem->SetShaderUniform(m_hDefaultUIShader, s_UIShaderLocations.texture_maps, 0, pFont->m_InternalInstanceID, &hFontAtlas);

		// Setup the font atlas.
		pFont->m_pInternalFontAtlasData = new InternalFontAtlasData_t;
		InternalFontAtlasData_t* pAtlas = reinterpret_cast<InternalFontAtlasData_t*>(pFont->m_pInternalFontAtlasData);
		pAtlas->charData = fontResource.charData;
		pAtlas->quadData = fontResource.charQuads;

		pFontLookup->FontIndex = fontHandle;
	}
	else
	{
		// The font is already loaded, just return the handle.
		fontHandle = pFontLookup->FontIndex;
	}

	return fontHandle;
}

//-------------------------------------------------------------------------
// Loads a font at the provided path if it hasn't been loaded.
//-------------------------------------------------------------------------
FontHandle_t CFontSystem::AcquireFontResources_OLD(const char* pFontPath)
{
	return 0;
}

// Shoutouts to shreyaspranav for the stb_truetype example! :)
// https://github.com/shreyaspranav/stb-truetype-example/blob/main/Main.cpp
// TODO: Handle escape sequences here
GeometryConfig2D_t CFontSystem::AcquireConfigForText(const char* pText, FontHandle_t hFont)
{
	// TODO: Handle invalid fonts
	GeometryConfig2D_t newConfig{};
	newConfig.Vertices.Resize(1024); // TODO: Resize is still broken and causing issues with our allocator. We need to fix this.

	InternalFontAtlasData_t* pInternalAtlas = reinterpret_cast<InternalFontAtlasData_t*>(m_FontDictionary.m_FontArray[hFont].m_pInternalFontAtlasData);
	uint32_t currVertIndex{ 0 };

	// TODO: Configurable position and size
	Vector2D position{ -0.9f, -0.9f };
	float flSize{ 0.75f };
	float flPixelScale = 2.0f / 600.0f; // 600 is the default window height. TODO: CONFIGURABLE!
	
	int vertOrder[6] = { 0, 1, 2, 0, 2, 3 };
	
	size_t stringLength = StringLength(pText);

	for (int ch = 0; ch < stringLength; ch++)
	{
		// Make sure the character is in the font atlas.
		if (pText[ch] < kCodePointOfFirstChar || pText[ch] > kCodePointOfFirstChar + kCharsToIncludeInFontAtlas)
		{
			CAssert(0);
			continue;
		}

		// Get the data that is used in the rendering of this character.
		FontLoader::FontChar_t* pCurrPackedChar = &pInternalAtlas->charData[pText[ch] - kCodePointOfFirstChar];
		FontLoader::FontQuadData_t* pCurrAlignedQuad = &pInternalAtlas->quadData[pText[ch] - kCodePointOfFirstChar];

		// Scale the units above from pixels by flPixelScale.
		Vector2D vecGlyphSize =
		{
			(pCurrPackedChar->xMax - pCurrPackedChar->xMin) * flPixelScale * flSize,
			(pCurrPackedChar->yMax - pCurrPackedChar->yMin) * flPixelScale * flSize
		};

		Vector2D vecGlyphBoundingBoxBottomLeft =
		{
			position.x + (pCurrPackedChar->xOffset * flPixelScale * flSize),
			position.y - (pCurrPackedChar->yOffset + pCurrPackedChar->yMax - pCurrPackedChar->yMin) * flPixelScale * flSize
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
			{ pCurrAlignedQuad->sMax, pCurrAlignedQuad->tMin },
			{ pCurrAlignedQuad->sMin, pCurrAlignedQuad->tMin },
			{ pCurrAlignedQuad->sMin, pCurrAlignedQuad->tMax },
			{ pCurrAlignedQuad->sMax, pCurrAlignedQuad->tMax }
		};

		// Set up all of our vertex data.
		for (int idx = 0; idx < 4; idx++)
		{
			newConfig.Vertices[currVertIndex + idx].Pos = vecGlyphVertices[idx];
			newConfig.Vertices[currVertIndex + idx].Color = Vector4D(1.0f, 1.0f, 1.0f, 1.0f);
			newConfig.Vertices[currVertIndex + idx].TexCoord = vecGlyphTexCoords[idx];
		}

		// Fill up the index config based on the 'orders' array above.
		for (int idx = 0; idx < 6; idx++)
		{
			newConfig.Indices.PushBack(vertOrder[idx] + currVertIndex);
		}

		currVertIndex += 4;

		// Push the current position forward by the current character's advance.
		position.x += pCurrPackedChar->Advance * flPixelScale * flSize;
	}
	newConfig.Vertices.SetCount(currVertIndex);

	return newConfig;
}

FontHandle_t CFontSystem::GetFirstFreeFontHandle() const
{
	// Get the first free font slot.
	for (int fontIdx = 0; fontIdx < m_Config.MaxFonts; fontIdx++)
	{
		if (!m_FontDictionary.m_FontArray[fontIdx].m_pInternalFontAtlasData)
		{
			// Found one!
			return fontIdx;
		}
	}

	LOG_ERROR("No free slot was found in the font dictionary! Returning invalid ID");
	return kInvalidID_U32;
}