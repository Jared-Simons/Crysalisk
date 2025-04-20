//=============================================================================
// Author: Jared Simons
// Date Created: 4/19/2025
// 
// Purpose: The texture manager.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

struct FontSystemConfig_t
{
	size_t	MaxFonts{ 0 };
};

class IFontSystem
{
public:
	virtual bool	Initialize(FontSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual FontHandle_t	AcquireFontResources(const char* pFontPath) = 0;

	virtual GeometryConfig2D_t	AcquireConfigForText(const char* pText, FontHandle_t hFont) = 0;
};

DLL_EXPORT IFontSystem* g_pFontSystem;