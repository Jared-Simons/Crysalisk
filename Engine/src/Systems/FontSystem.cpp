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

	virtual bool	AcquireFontResources(const char* pFontPath) override;

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
bool CFontSystem::AcquireFontResources(const char* pFontPath)
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
		newFontConfig.pszDiffuseTextureName = (char*)pFontPath;
		newFontConfig.pszMaterialName = StringDuplicate("NewFontMaterialPogchampandwhatnot");
		newFontConfig.pszShaderName = StringDuplicate("Core.Phong"); // TODO: UI SHADER
		MaterialHandle_t newMaterial = g_pMaterialSystemInternal->AcquireMaterialFromConfig(newFontConfig);
	}



	return true;
}
