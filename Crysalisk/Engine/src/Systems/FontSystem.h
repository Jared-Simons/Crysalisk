//=============================================================================
// Author: Jared Simons
// Date Created: 4/19/2025
// 
// Purpose: The texture manager.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

//-------------------------------------------------------------------------
// Configuration for the font system.
//-------------------------------------------------------------------------
struct FontSystemConfig_t
{
	// The maximum number of fonts that can be loaded at once.
	size_t	MaxFonts{ 0 };
};

//-------------------------------------------------------------------------
// A piece of texture that can be used for rendering.
//-------------------------------------------------------------------------
class TextObject
{
public:
	// Sets the actual text to be rendered.
	void				SetText(const char* pszText);

public:
	// The size of this text.
	float				m_flTextSize{ 0.0f };
	// The position of this text (in Screen-Space [1080x720]).
	Vector2D			m_vecPos{};
	// The text this element should be rendering.
	char*				m_pszText{ nullptr };
	// The font to be used in rendering.
	FontHandle_t		m_hFont{ kInvalidID_U32 };
	// The geometry associated with this text.
	GeometryInstance_t	m_Geometry{};
};

//-------------------------------------------------------------------------
// Interface for the font system.
//-------------------------------------------------------------------------
class IFontSystem
{
public:
	// Initialization, shutdown.
	virtual bool				Initialize(FontSystemConfig_t config) = 0;
	virtual void				Shutdown() = 0;

	// Acquires the resources for a given font.
	virtual FontHandle_t		AcquireFontResources(const char* pFontName) = 0;

	virtual TextureHandle_t		AcquireFontAtlasTexture(FontHandle_t hFont) const = 0;

	// Generates the geometry necessary to render a piece of text.
	virtual GeometryConfig2D_t	AcquireConfigForText(TextObject& pTextObject, FontHandle_t hFont, float flTextSizet) = 0;
	virtual GeometryConfig_t	Acquire3DConfigForText(FontHandle_t hFont, float flTextSize, const char* pszText) = 0;
};

// Global interface to the font system context.
DLL_EXPORT IFontSystem* g_pFontSystem;