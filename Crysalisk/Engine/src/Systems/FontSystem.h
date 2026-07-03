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
	u64	MaxFonts{ 0 };

	// A pointer to the renderer.
	class IRenderer* pRenderer{ nullptr };
	// A pointer to the texture system.
	class ITextureSystem* pTextureSystem{ nullptr };
	// A pointer to the shader system.
	class IShaderSystem* pShaderSystem{ nullptr };
};

//-------------------------------------------------------------------------
// A piece of text that can be used for rendering.
//-------------------------------------------------------------------------
class ENGINE_API TextObject
{
public:
	// Sets the actual text to be rendered.
	void				SetText(class IRenderer* pRenderer, class IFontSystem* pFontSystem, const char* pszText);

public:
	// The size of this text.
	f32					m_flTextSize{ 0.0f };
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
	static void					AllocateFontSystemState(IFontSystem** ppSystemState);

	// Initialization, shutdown.
	virtual bool				Initialize(FontSystemConfig_t config) = 0;
	virtual void				PostInitialize() = 0;
	virtual void				Shutdown() = 0;

	// Acquires the resources for a given font.
	virtual FontHandle_t		AcquireFontResources(const char* pFontName, f32 flFontSize) = 0;
	virtual void				ReleaseFontResources(FontHandle_t hFont) = 0;

	virtual TextureHandle_t		AcquireFontAtlasTexture(FontHandle_t hFont) const = 0;

	// Binds a font instance's resources for rendering.
	virtual void				BindFontInstance(FontHandle_t hFont) const = 0;

	// Generates the geometry necessary to render a piece of text.
	virtual GeometryConfig2D_t	AcquireConfigForText(FontHandle_t hFont, Vector2D vecTextPos, const char* pszText) = 0;
	virtual GeometryConfig2D_t	AcquireConfigForText(TextObject& pTextObject, FontHandle_t hFont, f32 flTextSizet) = 0;
	virtual GeometryConfig_t	Acquire3DConfigForText(FontHandle_t hFont, f32 flTextSize, const char* pszText) = 0;

	virtual FontHandle_t		GetDefaultFont() const = 0;
};
