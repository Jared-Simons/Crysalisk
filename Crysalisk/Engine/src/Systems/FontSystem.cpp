//=============================================================================
// Author: Jared Simons
// Date Created: 4/19/2025
// 
// Purpose: The font manager.
//=============================================================================
#include "FontSystem.h"

#include <Containers/CHashMap.h>
#include "Core/Engine.h"
#include <Core/Logging.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include <Resources/Loaders/FontLoader.h>
#include "Systems/TextureSystem.h"
#include "Systems/ShaderSystem.h"

//-------------------------------------------------------------------------
// Indices for each of the UI shader lookups.
//-------------------------------------------------------------------------
struct UIShaderLocations_t
{
	u16	projection{ kInvalidID_U16 };
	u16	view{ kInvalidID_U16 };
	u16	color{ kInvalidID_U16 };
	u16	texture_maps{ kInvalidID_U16 };
	u16	model{ kInvalidID_U16 };
};

// UI Shader Locations
static UIShaderLocations_t s_UIShaderLocations{};

// The codepoint of the first character we include in our font atlases.
const u32 kCodePointOfFirstChar = 32;
// The number of characters we include in our font atlases.
const u32 kCharsToIncludeInFontAtlas = 95;

// This is the resolution we are expecting when creating new text geometry.
// The font system will take care of scaling these appropriately to the current surface resolution.
const int kUIInputWidth = 640;
const int kUIInputHeight = 480;

//-------------------------------------------------------------------------
// Font System Implementation.
//-------------------------------------------------------------------------
class CFontSystem : public IFontSystem
{
	// Initialization, shutdown.
	virtual bool				Initialize(FontSystemConfig_t config) override;
	virtual void				PostInitialize() override;
	virtual void				Shutdown() override;

	// Acquires the resources for a given font.
	virtual FontHandle_t		AcquireFontResources(const char* pFontName) override;

	virtual TextureHandle_t		AcquireFontAtlasTexture(FontHandle_t hFont) const override;

	// Generates the geometry necessary to render a piece of text.
	virtual GeometryConfig2D_t	AcquireConfigForText(TextObject& pTextObject, FontHandle_t hFont, f32 flTextSize) override;
	virtual GeometryConfig_t	Acquire3DConfigForText(FontHandle_t hFont, f32 flTextSize, const char* pszText) override;

private:
	// Returns the first available handle in the font dictionary.
	FontHandle_t	GetFirstFreeFontHandle() const;

private:
	// The font system config passed in from the application configuration.
	FontSystemConfig_t m_Config{};

	// A lookup for a particular font.
	struct FontLookup
	{
		FontHandle_t	FontIndex{ kInvalidID_U32 };
	};

	struct FontDictionary
	{
		// Allows for string-based lookups for fonts.
		Utl::CStringHashMap<FontLookup>			m_FontHashmap{};
		// An array of fonts.
		Font*									m_FontArray{ nullptr };
	};

	RenderBuffer* m_pUIVertexBuffer{ nullptr };
	RenderBuffer* m_pUIIndexBuffer{ nullptr };

	// The font dictionary.
	FontDictionary	m_FontDictionary{};
	// The default UI shader, in case a different shader is not available.
	ShaderHandle_t	m_hDefaultUIShader{ kInvalidID_U32 };

	// The default font, used in case nothing else is available.
	FontHandle_t	m_hDefaultFont{ kInvalidID_U32 };

	// This needs access to our buffers.
	friend class TextObject;
};

// The internal font atlas data for a font.
// Can be accessed through pFont->m_pInternalData.
struct InternalFontAtlasData_t
{
	Utl::CVector<FontLoader::FontCharData_t>	glyphs{};
};

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IFontSystem::AllocateFontSystemState(IFontSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold font system state was not provided! Font system creation failed!");
		return;
	}

	*ppSystemState = new CFontSystem;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CFontSystem::Initialize(FontSystemConfig_t config)
{
	m_Config = config;
	m_FontDictionary.m_FontHashmap.Resize(config.MaxFonts);

	// Allocate memory for our font array
	m_FontDictionary.m_FontArray = (Font*)new Font[config.MaxFonts];

	m_pUIVertexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(Vertex2D) * 1 * 1024 * 1024, eRenderBufferUsage::VERTEX, true, "UI_Vertex_Buffer");
	if (m_pUIVertexBuffer == nullptr)
	{
		LOG_ERROR("UI View failed to initialize!");
		return false;
	}

	m_pUIIndexBuffer = m_Config.pRenderer->CreateRenderBuffer(sizeof(u32) * 1 * 1024 * 1024, eRenderBufferUsage::INDEX, true, "UI_Index_Buffer");
	if (m_pUIVertexBuffer == nullptr)
	{
		LOG_ERROR("UI View failed to initialize!");
		return false;
	}

	LOG_INFO("Font system initialized! (Max fonts: %d)", config.MaxFonts);
	return true;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CFontSystem::PostInitialize()
{
	// Set up our required shader resources.
	m_hDefaultUIShader = m_Config.pShaderSystem->AcquireShader("Core.UI");

	if (m_hDefaultUIShader == kInvalidID_U32)
	{
		LOG_ERROR("Failed to acquire necessary shader resources for font rendering!");
	}

	// Get all our shader uniform locations
	s_UIShaderLocations.projection = m_Config.pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "projection");
	s_UIShaderLocations.view = m_Config.pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "view");
	s_UIShaderLocations.color = m_Config.pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "color");
	s_UIShaderLocations.texture_maps = m_Config.pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "texture_maps");
	s_UIShaderLocations.model = m_Config.pShaderSystem->GetShaderUniformLocation(m_hDefaultUIShader, "model");

	// Load default font.
	m_hDefaultFont = AcquireFontResources("Assets/Fonts/UbuntuMono-R.ttf");
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CFontSystem::Shutdown()
{
	delete[] m_FontDictionary.m_FontArray;
}

//-------------------------------------------------------------------------
// Purpose: Attempts to acquire the resources for a given font.
// Input:	pszFontName - The font to acquire resources for.
// Output:	Returns a handle to the new font.
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
		return m_hDefaultFont;
	}

	// Check to see if the font is already loaded.
	pFontLookup = &m_FontDictionary.m_FontHashmap[pszFontName];

	if (pFontLookup->FontIndex == kInvalidID_U32)
	{
		// The font is not yet loaded, load it up.
		if (!FontLoader::LoadFontResource(pszFontName, 30, fontResource))
		{
			LOG_ERROR("Failed to load font resource data for font '%s'. Returning default font.", pszFontName);
			return m_hDefaultFont;
		}

		fontHandle = GetFirstFreeFontHandle();
		pFont = &m_FontDictionary.m_FontArray[fontHandle];
		pFont->m_iLineHeight = fontResource.lineHeight;

		// Load up the texture data.
		TextureHandle_t hFontAtlas = m_Config.pTextureSystem->AcquireTextureFromResource(fontResource.FontAtlasResource, pszFontName, true);

		// Get our shader instance data and bind the font atlas.
		m_Config.pShaderSystem->AcquireShaderInstanceResources(m_hDefaultUIShader, &pFont->m_InternalInstanceID, pszFontName);
		m_Config.pShaderSystem->SetShaderUniform(m_hDefaultUIShader, s_UIShaderLocations.texture_maps, 0, pFont->m_InternalInstanceID, &hFontAtlas);
		pFont->m_hFontTexture = hFontAtlas;

		// Setup the font atlas.
		pFont->m_pInternalFontAtlasData = new InternalFontAtlasData_t;
		InternalFontAtlasData_t* pAtlas = reinterpret_cast<InternalFontAtlasData_t*>(pFont->m_pInternalFontAtlasData);
		pAtlas->glyphs = fontResource.charGlyphs;

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
// Purpose: Returns a handle to the provided font's atlas texture.
// Input:	hFont - The font handle who's atlas to return.
// Output:	Returns a handle to the font's atlas texture.
//-------------------------------------------------------------------------
TextureHandle_t CFontSystem::AcquireFontAtlasTexture(FontHandle_t hFont) const
{
	if (hFont == kInvalidID_U32 || hFont > m_Config.MaxFonts)
	{
		LOG_ERROR("AcquireFontAtlasTexture: Provided font handle is invalid. Returning invalid handle.");
		return kInvalidID_U32;
	}

	return m_FontDictionary.m_FontArray[hFont].m_hFontTexture;
}

//-------------------------------------------------------------------------
// Purpose: Generates the geometry necessary to render a piece of text.
// Input:	pTextObject - The text object to acquire resources for.
//			hFont - The font associated with this text object.
// Output:	Returns a new geometry config to use in rendering the new string.
//-------------------------------------------------------------------------
GeometryConfig2D_t CFontSystem::AcquireConfigForText(TextObject& pTextObject, FontHandle_t hFont, f32 flTextSize)
{
	GeometryConfig2D_t			newConfig{};
	InternalFontAtlasData_t*	pInternalAtlas = reinterpret_cast<InternalFontAtlasData_t*>(m_FontDictionary.m_FontArray[hFont].m_pInternalFontAtlasData);
	FontLoader::FontCharData_t* pCurGlyph = pInternalAtlas->glyphs.Data();

	u64						iStringLength = Utl::StringLength(pTextObject.m_pszText);
	int							iCurNonWhitespaceCharCount = 0; 

	// Geometry data.
	Vector2D					vecCurPos = pTextObject.m_vecPos;
	f32						width, height;
	Vector2D					vecGlyphBottomLeftPos = {};
	Vector2D					vecCharVertices[4] = {};
	Vector2D					vecCharTexCoords[4] = {};

	for (int ch = 0; ch < iStringLength; ch++)
	{
		// If a space, just push the position forward by some amount.
		if (pTextObject.m_pszText[ch] == ' ')
		{
			vecCurPos.x += 6.0f * flTextSize;
			continue;
		}

		// Skip escape codes for now.
		// LEFTOFF: Implement
		if (pTextObject.m_pszText[ch] == '\n' || pTextObject.m_pszText[ch] == '\t')
		{
			// Escape chars!
			if (pTextObject.m_pszText[ch] == '\n')
			{
				vecCurPos.x = pTextObject.m_vecPos.x;
				vecCurPos.y += m_FontDictionary.m_FontArray[hFont].m_iLineHeight;

				continue;
			}
		}

		// Make sure the glyph is actually in the atlas.
		if (pTextObject.m_pszText[ch] < kCodePointOfFirstChar || pTextObject.m_pszText[ch] > kCodePointOfFirstChar + kCharsToIncludeInFontAtlas)
			continue;

		pCurGlyph = &pInternalAtlas->glyphs[pTextObject.m_pszText[ch] - kCodePointOfFirstChar]; 
		width = pCurGlyph->xMax - pCurGlyph->xMin;
		height = pCurGlyph->yMax - pCurGlyph->yMin;
		
		f32 flMinX = (vecCurPos.x + pCurGlyph->xOffset) * flTextSize;
		f32 flMinY = (vecCurPos.y + pCurGlyph->yOffset) * flTextSize;
		f32 flMaxX = (flMinX + width) * flTextSize;
		f32 flMaxY = (flMinY + height) * flTextSize;
		f32 flTexMinX = pCurGlyph->sMin;
		f32 flTexMaxX = pCurGlyph->sMax;
		// Flip the y-coord around.
		f32 flTexMinY = 1.0f - pCurGlyph->tMin;
		f32 flTexMaxY = 1.0f - pCurGlyph->tMax;

		// Vertices:
		vecCharVertices[0] = { flMinX, flMinY };
		vecCharVertices[1] = { flMaxX, flMinY };
		vecCharVertices[2] = { flMaxX, flMaxY };
		vecCharVertices[3] = { flMinX, flMaxY };

		// Vertex texcoords.
		vecCharTexCoords[0] = { flTexMinX, flTexMinY };
		vecCharTexCoords[1] = { flTexMaxX, flTexMinY };
		vecCharTexCoords[2] = { flTexMaxX, flTexMaxY };
		vecCharTexCoords[3] = { flTexMinX, flTexMaxY };

		// Add these to the geometry config.
		newConfig.Vertices.PushBack(Vertex2D{ vecCharVertices[0], Vector4D(1.0, 1.0, 1.0, 1.0), vecCharTexCoords[0] });
		newConfig.Vertices.PushBack(Vertex2D{ vecCharVertices[2], Vector4D(1.0, 1.0, 1.0, 1.0), vecCharTexCoords[2] });
		newConfig.Vertices.PushBack(Vertex2D{ vecCharVertices[3], Vector4D(1.0, 1.0, 1.0, 1.0), vecCharTexCoords[3] });
		newConfig.Vertices.PushBack(Vertex2D{ vecCharVertices[1], Vector4D(1.0, 1.0, 1.0, 1.0), vecCharTexCoords[1] });

		int indexOrder[6] = { 2, 1, 0, 3, 0, 1 };

		// Fill up the index buffer as well.
		for (int idx = 0; idx < 6; idx++)
		{
			newConfig.Indices.PushBack((4 * iCurNonWhitespaceCharCount) + indexOrder[idx]);
		}

		// Advance the x-position over.
		vecCurPos.x += pCurGlyph->flAdvance * flTextSize;

		// NOTE: This is needed to ensure the index buffer is properly setup for characters that directly follow escape codes...
		// If we use the 'ch' iterator instead, our index buffer will be offset to the wrong character
		// as 'ch' includes escape codes, while 'iCurNonWhitespaceCharCount' does not.
		iCurNonWhitespaceCharCount++;
	}

	return newConfig;
}

//-------------------------------------------------------------------------
// Purpose: Generates the geometry necessary to render a piece of text in the 3D scene.
// Input:	hFont - The font associated with this text object.
//			flTextSize - The size of the text.
//			pszText - The text to acquire geometry for.
// Output:	Returns a new geometry config to use in rendering the new text.
//-------------------------------------------------------------------------
GeometryConfig_t CFontSystem::Acquire3DConfigForText(FontHandle_t hFont, f32 flTextSize, const char* pszText)
{
	GeometryConfig_t				newConfig{};
	InternalFontAtlasData_t*		pInternalAtlas = reinterpret_cast<InternalFontAtlasData_t*>(m_FontDictionary.m_FontArray[hFont].m_pInternalFontAtlasData);
	FontLoader::FontCharData_t*		pCurGlyph = pInternalAtlas->glyphs.Data();

	u64						iStringLength = Utl::StringLength(pszText);
	int							iCurNonWhitespaceCharCount = 0;
	Vector3D					vecCurPos{ 0.0f, 0.0f, 0.0f };

	// TODO: Isn't this kinda arbitrary?
	f32						flTextScale = flTextSize / 128.0f;

	// Geometry data.
	Vector2D					vecGlyphSize = {};
	Vector3D					vecGlyphBottomLeftPos = {};
	Vector3D					vecCharVertices[4] = {};
	Vector2D					vecCharTexCoords[4] = {};

	for (int ch = 0; ch < iStringLength; ch++)
	{
		pCurGlyph = &pInternalAtlas->glyphs[pszText[ch] - kCodePointOfFirstChar];

		// Vertex positions: top-right, top-left, bottom-left, bottom-right.
		vecCharVertices[0] = Vector{ (100.0f * ch) + 50.0f, 100.0f, 0.0f };
		vecCharVertices[1] = Vector{ (100.0f * ch), 100.0f, 0.0f };
		vecCharVertices[2] = Vector{ (100.0f * ch), 0.0f, 0.0f };
		vecCharVertices[3] = Vector{ (100.0f * ch) + 50.0f, 0.0f, 0.0f };

		// Scale all our vertex positions.
		for (int idx = 0; idx < 4; idx++)
		{
			vecCharVertices[idx] *= flTextScale;
		}

		// Vertex TexCoords.
		vecCharTexCoords[0] = { pCurGlyph->sMax, pCurGlyph->tMin };
		vecCharTexCoords[1] = { pCurGlyph->sMin, pCurGlyph->tMin };
		vecCharTexCoords[2] = { pCurGlyph->sMin, pCurGlyph->tMax };
		vecCharTexCoords[3] = { pCurGlyph->sMax, pCurGlyph->tMax };

		// Add these to the geometry config.
		newConfig.Vertices.PushBack(Vertex3D{ vecCharVertices[0], Vector3D(1.0, 1.0, 1.0), vecCharTexCoords[0] });
		newConfig.Vertices.PushBack(Vertex3D{ vecCharVertices[1], Vector3D(1.0, 1.0, 1.0), vecCharTexCoords[1] });
		newConfig.Vertices.PushBack(Vertex3D{ vecCharVertices[2], Vector3D(1.0, 1.0, 1.0), vecCharTexCoords[2] });
		newConfig.Vertices.PushBack(Vertex3D{ vecCharVertices[3], Vector3D(1.0, 1.0, 1.0), vecCharTexCoords[3] });

		int indexOrder[6] = { 0, 1, 2, 0, 2, 3 };

		// Fill up the index buffer as well.
		for (int idx = 0; idx < 6; idx++)
		{
			newConfig.Indices.PushBack((4 * iCurNonWhitespaceCharCount) + indexOrder[idx]);
		}

		// Advance the x-position over.
		vecCurPos.x += pCurGlyph->flAdvance * flTextScale;

		// NOTE: This is needed to ensure the index buffer is properly setup for characters that directly follow escape codes...
		// If we use the 'ch' iterator instead, our index buffer will be offset to the wrong character
		// as 'ch' includes escape codes, while 'iCurNonWhitespaceCharCount' does not.
		iCurNonWhitespaceCharCount++;
	}

	return newConfig;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available handle in the font dictionary.
// Input:	none.
// Output:	Returns the first available font handle.
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// Purpose: Sets the actual text to be rendered with this text object.
// Input:	pszText - The new string to be rendered with this object.
// Output:	none.
//-------------------------------------------------------------------------
void TextObject::SetText(class IRenderer* pRenderer, class IFontSystem* pFontSystem, const char* pszText)
{
	// Can't render text without text to render!
	if (!pszText || Utl::StringLength(pszText) < 1)
		return;

	bool bCreateNewResources = (m_pszText == nullptr);

	if (m_pszText)
		Utl::StringFree(m_pszText);
	m_pszText = Utl::StringDuplicate(pszText);

	// Get new geometry for our text.
	GeometryConfig2D_t newConfig = pFontSystem->AcquireConfigForText(*this, m_hFont, m_flTextSize);
	CFontSystem* pTypedSystem = dynamic_cast<CFontSystem*>(pFontSystem);

	// Acquire new space on the GPU only if there was no text (therefore resources) prior.
	// Otherwise, just update the existing data.
	if (bCreateNewResources)
	{
		pRenderer->AcquireGeometryResources(*pTypedSystem->m_pUIVertexBuffer, *pTypedSystem->m_pUIIndexBuffer,
			newConfig.Vertices.Count(), newConfig.Vertices.Stride(), 0, (void*)newConfig.Vertices.Data(),
			newConfig.Indices.Count(), (void*)newConfig.Indices.Data(), &m_Geometry);
	}
	else
	{
		pRenderer->UpdateGeometryResources(*pTypedSystem->m_pUIVertexBuffer, *pTypedSystem->m_pUIIndexBuffer, m_Geometry, newConfig);
	}
}
